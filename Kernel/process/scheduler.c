#include <stddef.h>
#include <scheduler.h>
#include <stdbool.h>
#include <queueADT.h>
#include <interrupts.h>
#include <memoryManager.h>

typedef struct scheduler_state {
    process_t *current;
    scheduler_metrics_t metrics;
    queue_t *ready_queues[SCHEDULER_PRIORITY_LEVELS];
    process_t *idle;
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
    _cli();
    size_t index = priority_index(priority);
    queue_t *queue = scheduler.ready_queues[index];
    if (queue == NULL) {
        queue = queue_create();
        scheduler.ready_queues[index] = queue;
    }
    _sti();
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
    _cli();
    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *queue = queue_create();
        scheduler.ready_queues[i] = queue;
    }

    char **argv_idle = mem_alloc(sizeof(char *));
    argv_idle[0] = "idle";
    scheduler.idle = createProcess(1, argv_idle, 0, SCHEDULER_MIN_PRIORITY, 0, idle_process_entry);
    _sti();
}

void scheduler_add_ready(process_t *process) {
    _cli();
    if (process == NULL) {
        _sti();
        return;
    }
    process->state = PROCESS_STATE_READY;
    if (process != scheduler.idle) {
        process->priority = priority_from_usage(process->last_quantum_ticks);
    }
    process->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    process->last_quantum_ticks = 0;
    queue_push(queue_for_priority(process->priority), process);
    _sti();
}

bool scheduler_remove_ready(process_t *process) {
    _cli();
    if (process == NULL || process == scheduler.idle) {
        _sti();
        return false;
    }

    queue_t *queue = queue_for_priority(process->priority);
    if (queue != NULL && queue_remove(queue, process)) {
        _sti();
        return true;
    }

    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *other_queue = scheduler.ready_queues[i];
        if (other_queue != NULL && other_queue != queue && queue_remove(other_queue, process)) {
            _sti();
            return true;
        }
    }
    _sti();

    return false;
}

process_t *scheduler_current(void) {
    return scheduler.current;
}

void scheduler_clear_current(process_t *process) {
    if (process != NULL && scheduler.current == process) {
        scheduler.current = NULL;
    }
}


void *schedule_tick(void *current_rsp) {
    scheduler.metrics.total_ticks++;

    process_t *running = scheduler.current;
    bool must_switch = true;

    if (running != NULL) {
        running->context.rsp = (uint64_t)current_rsp;
        if (running == scheduler.idle) {
            // allow the idle task to yield every tick but still prefer real work
            running->state = PROCESS_STATE_READY;
        } else {
            if(running->state != PROCESS_STATE_RUNNING) {
                if (running->state == PROCESS_STATE_YIELD) {
                    scheduler_add_ready(running);
                }
                // The running process was blocked, yielded, or terminated during its time slice
                scheduler.current = NULL;
                process_set_running(NULL);
                must_switch = true;
            } else if (running->remaining_quantum > 0) {
                running->remaining_quantum--;
                running->last_quantum_ticks = (uint8_t)(SCHEDULER_DEFAULT_QUANTUM - running->remaining_quantum);
                must_switch = false;
            } else {
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
        process_t *next = queue_pop(queue);
        if (next != NULL && next->state == PROCESS_STATE_READY) {
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
            process_t *process = queue_iter_next(&it);
            callback(process, context);
        }
    }
}

int32_t scheduler_set_process_priority(uint32_t pid, uint8_t priority) {
    _cli();
    if (priority < SCHEDULER_MAX_PRIORITY || priority > SCHEDULER_MIN_PRIORITY) {
        _sti();
        return -1;
    }

    process_t *process = process_lookup(pid);
    if (process == NULL) {
        _sti();
        return -1;
    }

    if (process->state == PROCESS_STATE_TERMINATED) {
        _sti();
        return -1;
    }

    if (process == scheduler.idle) {
        _sti();
        return -1;
    }

    uint8_t old_priority = process->priority;

    if (process->state == PROCESS_STATE_READY && priority != old_priority) {
        queue_t *source_queue = queue_for_priority(old_priority);
        _cli();
        queue_t *buffer_queue = queue_create();
        if (buffer_queue == NULL) {
            _sti();
            return -1;
        }

        process_t *entry = NULL;
        while ((entry = queue_pop(source_queue)) != NULL) {
            if (entry != process) {
                queue_push(buffer_queue, entry);
            }
        }

        while ((entry = queue_pop(buffer_queue)) != NULL) {
            queue_push(source_queue, entry);
        }
        queue_destroy(buffer_queue, NULL);
    }

    process->priority = priority;

    if (process->state == PROCESS_STATE_READY && priority != old_priority) {
        queue_push(queue_for_priority(priority), process);
        _cli();
    }

    if (process == scheduler.current) {
        process->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    }
    _sti();

    return 0;
}
