#include <sem.h>
#include <queueADT.h>
#include <stdint.h>
#include <lib.h>
#include <memoryManager.h>
#include <strings.h>
#include <process.h>
#include <scheduler.h>
#include <interrupts.h>

static queue_t *registered_semaphores = NULL;
static uint8_t registry_lock = 0;

static bool timer_tick_is_disabled(void) {
    return (picMasterGetMask() & 0x01) != 0;
}

static void ensure_registry(void) {
    if (registered_semaphores == NULL) {
        registered_semaphores = queue_create();
    }
}

static sem_t *find_registered(const char *name) {
    if (registered_semaphores == NULL || name == NULL) {
        return NULL;
    }

    queue_iterator_t it = queue_iter(registered_semaphores);
    while (queue_iter_has_next(&it)) {
        sem_t *candidate = queue_iter_next(&it);
        if (candidate != NULL && candidate->name != NULL && strcmp(candidate->name, name) == 0) {
            return candidate;
        }
    }
    return NULL;
}

sem_t *sem_create(void) {
    return mem_alloc(sizeof(sem_t));
}

sem_t *sem_find(const char *name) {
    sem_t *result = NULL;
    semLock(&registry_lock);
    result = find_registered(name);
    semUnlock(&registry_lock);
    return result;
}

void sem_init(sem_t *sem, const char *name, uint32_t initial_count){
    if(sem == NULL || name == NULL){
        return;
    }

    ensure_registry();

    sem->name = mem_alloc(strlen(name) + 1);
    if (sem->name == NULL) {
        return;
    }
    strcpy(sem->name, name);
    sem->count = initial_count;
    sem->waiting_processes = queue_create();
    sem->lock = 0;

    semLock(&registry_lock);
    if (find_registered(name) == NULL) {
        queue_push(registered_semaphores, sem);
    }
    semUnlock(&registry_lock);
}

void sem_destroy(sem_t *sem) {
    if (sem == NULL) {
        return;
    }

    semLock(&registry_lock);
    if (registered_semaphores != NULL) {
        queue_remove(registered_semaphores, sem);
    }
    semUnlock(&registry_lock);

    semLock(&sem->lock);

    while (sem->waiting_processes != NULL && !queue_is_empty(sem->waiting_processes)) {
        process_t *process = (process_t *)queue_pop(sem->waiting_processes);
        if (process != NULL) {
            process_unblock(process);
        }
    }

    queue_destroy(sem->waiting_processes, NULL);
    sem->waiting_processes = NULL;
    sem->count = 0;

    semUnlock(&sem->lock);
    sem->lock = 0;

    if (sem->name != NULL) {
        mem_free(sem->name);
        sem->name = NULL;
    }
}

int sem_post(sem_t *sem){
    int ret = -1;
    bool should_force_scheduler = false;
    if(sem == NULL){
        return ret;
    }

    semLock(&sem->lock);
    
    if(queue_is_empty(sem->waiting_processes)){
        sem->count++;
        ret = 0;
    } else {
        process_t *process = (process_t *)queue_pop(sem->waiting_processes);
        if (process != NULL) {
            should_force_scheduler = process_unblock(process);
            ret = 0;
        }
    }

    semUnlock(&sem->lock);

    if (should_force_scheduler && timer_tick_is_disabled()) {
        process_yield();
    }

    return ret;
}

int sem_wait(sem_t *sem){
    int ret = -1;
    bool blocked = false;
    if(sem == NULL){
        return ret;
    }
    
    semLock(&sem->lock);

    if(sem->count > 0){
        sem->count--;
        ret = 0;
    } else {
        process_t *current_process = scheduler_current();
        if (current_process != NULL) {
            queue_push(sem->waiting_processes, current_process);
            blocked = process_block(current_process);
            ret = 0;
        }
    }

    semUnlock(&sem->lock);

    if (blocked) {
        _force_scheduler_interrupt();
    }

    return ret;
}

int sem_waiting_count(sem_t *sem) {
    if (sem == NULL) {
        return -1;
    }

    int count = 0;
    semLock(&sem->lock);
    if (sem->waiting_processes != NULL) {
        count = (int)queue_size(sem->waiting_processes);
    }
    semUnlock(&sem->lock);
    return count;
}

int sem_get_value(sem_t *sem) {
    if (sem == NULL) {
        return -1;
    }
    semLock(&sem->lock);
    int value = (int)sem->count;
    semUnlock(&sem->lock);
    return value;
}

int sem_remove_process(sem_t *sem, int pid) {
    if (sem == NULL) {
        return -1;
    }

    return queue_remove(sem->waiting_processes, (void *)(uintptr_t)pid);
}

int sem_set_value(sem_t *sem, uint32_t new_value) {
    if (sem == NULL) {
        return -1;
    }

    semLock(&sem->lock);
    uint32_t current = sem->count;
    semUnlock(&sem->lock);

    if (new_value > current) {
        uint32_t delta = new_value - current;
        for (uint32_t i = 0; i < delta; i++) {
            sem_post(sem);
        }
    } else if (new_value < current) {
        semLock(&sem->lock);
        sem->count = new_value;
        semUnlock(&sem->lock);
    }

    return 0;
}
