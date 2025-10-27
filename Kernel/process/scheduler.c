#include <stddef.h>
#include <scheduler.h>
#include <stdbool.h>
#include <queueADT.h>
#include <interrupts.h>
#include <memoryManager.h>

typedef struct scheduler_state {
    pcb_t *current;
    scheduler_metrics_t metrics;
    queue_t *ready_queues[SCHEDULER_PRIORITY_LEVELS];
    pcb_t *idle;
} scheduler_state_t;

static scheduler_state_t scheduler = {0};

static void idle_process_entry(void) {
    while (1) {
        _hlt();
    }
}

static size_t priority_index(uint8_t priority) {
    if (priority < SCHEDULER_MAX_PRIORITY) {
        priority = SCHEDULER_MAX_PRIORITY;
    } else if (priority > SCHEDULER_MIN_PRIORITY) {
        priority = SCHEDULER_MIN_PRIORITY;
    }
    return priority - SCHEDULER_MAX_PRIORITY;
}

static queue_t *queue_for_priority(uint8_t priority) {
    size_t index = priority_index(priority);
    queue_t *queue = scheduler.ready_queues[index];
    if (queue == NULL) {
        queue = queue_create();
        scheduler.ready_queues[index] = queue;
    }
    return queue;
}

static uint8_t priority_from_usage(uint8_t ticks_used) {
    if (ticks_used > SCHEDULER_DEFAULT_QUANTUM) {
        ticks_used = SCHEDULER_DEFAULT_QUANTUM;
    }

    const uint8_t levels = SCHEDULER_PRIORITY_LEVELS;
    const uint32_t scaled_usage = (uint32_t)ticks_used * levels;

    for (uint8_t i = 0; i < levels; i++) {
        uint32_t threshold = (uint32_t)SCHEDULER_DEFAULT_QUANTUM * (i + 1);
        if (scaled_usage < threshold) {
            return (uint8_t)(SCHEDULER_MAX_PRIORITY + i);
        }
    }

    return (uint8_t)(SCHEDULER_MAX_PRIORITY + levels - 1);
}

void scheduler_init(void) {
    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *queue = queue_create();
        scheduler.ready_queues[i] = queue;
    }

    char **argv_idle = mem_alloc(sizeof(char *));
    argv_idle[0] = "idle";
    scheduler.idle = createProcess(1, argv_idle, 0, SCHEDULER_MIN_PRIORITY, 0, idle_process_entry);
}

void scheduler_add_ready(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    pcb->state = PROCESS_STATE_READY;
    if (pcb != scheduler.idle) {
        pcb->priority = priority_from_usage(pcb->last_quantum_ticks);
    }
    pcb->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    pcb->last_quantum_ticks = 0;
    queue_push(queue_for_priority(pcb->priority), pcb);
}

bool scheduler_remove_ready(pcb_t *pcb) {
    if (pcb == NULL || pcb == scheduler.idle) {
        return false;
    }

    queue_t *queue = queue_for_priority(pcb->priority);
    if (queue != NULL && queue_remove(queue, pcb)) {
        return true;
    }

    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *other_queue = scheduler.ready_queues[i];
        if (other_queue != NULL && other_queue != queue && queue_remove(other_queue, pcb)) {
            return true;
        }
    }

    return false;
}

pcb_t *scheduler_current(void) {
    return scheduler.current;
}

void scheduler_clear_current(pcb_t *pcb) {
    if (pcb != NULL && scheduler.current == pcb) {
        scheduler.current = NULL;
    }
}


void *schedule_tick(void *current_rsp) {
    scheduler.metrics.total_ticks++;

    pcb_t *running = scheduler.current;
    bool must_switch = true;

    if (running != NULL) {
        running->context.rsp = (uint64_t)current_rsp;
        if (running == scheduler.idle) {
            // allow the idle task to yield every tick but still prefer real work
            running->state = PROCESS_STATE_READY;
        } else {
            if(running->state != PROCESS_STATE_RUNNING) {
                // The running process was blocked or terminated during its time slice
                scheduler.current = NULL;
                process_set_running(NULL);
                must_switch = true;
            } else if (running->remaining_quantum > 0) {
                running->remaining_quantum--;
                running->last_quantum_ticks = (uint8_t)(SCHEDULER_DEFAULT_QUANTUM - running->remaining_quantum);
                must_switch = false;
            } else {
                running->state = PROCESS_STATE_READY;
                scheduler_add_ready(running);
                scheduler.current = NULL;
                process_set_running(NULL);
                must_switch = true;
            }
        }
    }

    if (!must_switch) {
        process_set_running(scheduler.current);
        return current_rsp;
    }

    for (uint8_t priority = SCHEDULER_MAX_PRIORITY; priority <= SCHEDULER_MIN_PRIORITY; priority++) {
        queue_t *queue = queue_for_priority(priority);
        pcb_t *next = queue_pop(queue);
        if (next != NULL) {
            if (next == scheduler.idle) {
                scheduler.idle->state = PROCESS_STATE_READY;
                continue;
            }
            scheduler.current = next;
            scheduler.current->state = PROCESS_STATE_RUNNING;
            scheduler.current->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM - 1;
            scheduler.current->last_quantum_ticks = 1;
            scheduler.metrics.context_switches++;
            process_set_running(scheduler.current);
            return (void *)next->context.rsp;
        }
    }

    if (scheduler.idle != NULL) {
        scheduler.idle->context.rsp = (uint64_t)current_rsp;
        scheduler.idle->state = PROCESS_STATE_RUNNING;
        if (scheduler.current != scheduler.idle) {
            scheduler.metrics.context_switches++;
        }
        scheduler.current = scheduler.idle;
        process_set_running(scheduler.idle);
        return (void *)scheduler.idle->context.rsp;
    }

    process_set_running(NULL);
    return current_rsp;
}



const scheduler_metrics_t *scheduler_get_metrics(void) {
    return &scheduler.metrics;
}
void scheduler_for_each_ready(scheduler_iter_cb callback, void *context) {
    if (callback == NULL) {
        return;
    }

    for (uint8_t priority = SCHEDULER_MAX_PRIORITY; priority <= SCHEDULER_MIN_PRIORITY; priority++) {
        queue_t *queue = queue_for_priority(priority);
        if (queue_is_empty(queue)) {
            continue;
        }
        queue_iterator_t it = queue_iter(queue);
        while (queue_iter_has_next(&it)) {
            pcb_t *pcb = queue_iter_next(&it);
            callback(pcb, context);
        }
    }
}

int32_t scheduler_set_process_priority(uint64_t pid, uint8_t priority) {
    if (priority < SCHEDULER_MAX_PRIORITY || priority > SCHEDULER_MIN_PRIORITY) {
        return -1;
    }

    pcb_t *pcb = process_lookup(pid);
    if (pcb == NULL) {
        return -1;
    }

    if (pcb->state == PROCESS_STATE_TERMINATED) {
        return -1;
    }

    if (pcb == scheduler.idle) {
        return -1;
    }

    uint8_t old_priority = pcb->priority;

    if (pcb->state == PROCESS_STATE_READY && priority != old_priority) {
        queue_t *source_queue = queue_for_priority(old_priority);
        queue_t *buffer_queue = queue_create();
        if (buffer_queue == NULL) {
            return -1;
        }

        pcb_t *entry = NULL;
        while ((entry = queue_pop(source_queue)) != NULL) {
            if (entry != pcb) {
                queue_push(buffer_queue, entry);
            }
        }

        while ((entry = queue_pop(buffer_queue)) != NULL) {
            queue_push(source_queue, entry);
        }
        queue_destroy(buffer_queue, NULL);
    }

    pcb->priority = priority;

    if (pcb->state == PROCESS_STATE_READY && priority != old_priority) {
        queue_push(queue_for_priority(priority), pcb);
    }

    if (pcb == scheduler.current) {
        pcb->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    }

    return 0;
}
