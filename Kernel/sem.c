#include <sem.h>
#include <queueADT.h>
#include <stdint.h>
#include <lib.h>
#include <memoryManager.h>
#include <strings.h>
#include <process.h>
#include <scheduler.h>

static queue_t *registered_semaphores = NULL;
static uint8_t registry_lock = 0;

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
        pcb_t *pcb = (pcb_t *)queue_pop(sem->waiting_processes);
        if (pcb != NULL) {
            process_unblock(pcb);
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
    if(sem == NULL){
        return ret;
    }

    semLock(&sem->lock);
    
    if(queue_is_empty(sem->waiting_processes)){
        sem->count++;
        ret = 0;
    } else {
        pcb_t *pcb = (pcb_t *)queue_pop(sem->waiting_processes);
        if (pcb != NULL) {
            process_unblock(pcb);
            ret = 0;
        }
    }

    semUnlock(&sem->lock);

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
        pcb_t *current_process = scheduler_current();
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

    int count = -1;
    semLock(&sem->lock);
    if (sem->waiting_processes != NULL) {
        count = (int)queue_size(sem->waiting_processes);
    }
    semUnlock(&sem->lock);
    return count;
}
