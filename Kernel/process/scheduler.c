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

#define SHELL_PRIORITY_THRESHOLD (SCHEDULER_MAX_PRIORITY + 1)

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
    uint64_t flags = interrupts_save_and_disable();
    size_t index = priority_index(priority);
    queue_t *queue = scheduler.ready_queues[index];
    if (queue == NULL) {
        queue = queue_create();
        scheduler.ready_queues[index] = queue;
    }
    interrupts_restore(flags);
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

static bool scheduler_should_age(process_t *process) {
    if (process == NULL) {
        return false;
    }
    if (process == scheduler.idle) {
        return false;
    }
    if (process->priority <= SCHEDULER_MAX_PRIORITY) {
        return false;
    }

    uint64_t waited = scheduler.metrics.total_ticks - process->ready_since_tick;
    return waited >= SCHEDULER_AGING_THRESHOLD;
}

static void scheduler_age_ready(void) {
    for (uint8_t priority = SCHEDULER_MAX_PRIORITY + 1; priority <= SCHEDULER_MIN_PRIORITY; priority++) {
        queue_t *queue = queue_for_priority(priority);
        size_t items = queue_size(queue);
        for (size_t i = 0; i < items; i++) {
            process_t *process = queue_pop(queue);
            if (process == NULL) {
                break;
            }

            if (process->state == PROCESS_STATE_READY && scheduler_should_age(process)) {
                process->priority--;
                if (process->is_shell && process->priority < SCHEDULER_MAX_PRIORITY) {
                    process->priority = SCHEDULER_MAX_PRIORITY;
                }
                process->ready_since_tick = scheduler.metrics.total_ticks;
                queue_push(queue_for_priority(process->priority), process);
            } else {
                queue_push(queue, process);
            }
        }
    }
}

void scheduler_init(void) {
    uint64_t flags = interrupts_save_and_disable();
    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *queue = queue_create();
        scheduler.ready_queues[i] = queue;
    }

    char **argv_idle = mem_alloc(sizeof(char *));
    argv_idle[0] = "idle";
    scheduler.idle = createProcess(1, argv_idle, 0, SCHEDULER_MIN_PRIORITY, 0, idle_process_entry);
    interrupts_restore(flags);
}

void scheduler_add_ready(process_t *process) {
    uint64_t flags = interrupts_save_and_disable();
    if (process == NULL) {
        interrupts_restore(flags);
        return;
    }
    /* Ensure the process is not already enqueued (avoid duplicates) */
    if (process->state == PROCESS_STATE_READY) {
        scheduler_remove_ready(process);
    }
    process->state = PROCESS_STATE_READY;
    if (process != scheduler.idle) {
        if (process->priority_fixed) {
            process->priority = process->priority_requested;
        } else {
            process->priority = priority_from_usage(process->last_quantum_ticks);
            if (process->is_shell && process->priority > SHELL_PRIORITY_THRESHOLD) {
                process->priority = SHELL_PRIORITY_THRESHOLD;
            }
        }
    }
    process->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    process->last_quantum_ticks = 0;
    process->ready_since_tick = scheduler.metrics.total_ticks;
    queue_push(queue_for_priority(process->priority), process);
    interrupts_restore(flags);
}

bool scheduler_remove_ready(process_t *process) {
    if (process == NULL || process == scheduler.idle) {
        return false;
    }

    uint64_t flags = interrupts_save_and_disable();
    bool removed = false;

    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *queue = scheduler.ready_queues[i];
        if (queue == NULL) {
            continue;
        }

        /* Strip every occurrence so we do not leave stale duplicates behind */
        while (queue_remove(queue, process)) {
            removed = true;
        }
    }

    interrupts_restore(flags);
    return removed;
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
            running->state = PROCESS_STATE_READY;
        } else {
            if(running->state != PROCESS_STATE_RUNNING) {
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

    scheduler_age_ready();

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
    uint64_t flags = interrupts_save_and_disable();
    if (priority < SCHEDULER_MAX_PRIORITY || priority > SCHEDULER_MIN_PRIORITY) {
        interrupts_restore(flags);
        return -1;
    }

    process_t *process = process_lookup(pid);
    if (process == NULL) {
        interrupts_restore(flags);
        return -1;
    }

    if (process->state == PROCESS_STATE_TERMINATED) {
        interrupts_restore(flags);
        return -1;
    }

    if (process == scheduler.idle) {
        interrupts_restore(flags);
        return -1;
    }

    if (process->is_shell && priority > SHELL_PRIORITY_THRESHOLD) {
        interrupts_restore(flags);
        return -1;
    }

    uint8_t target_priority = priority;

    uint8_t old_priority = process->priority;

    if (process->state == PROCESS_STATE_READY && target_priority != old_priority) {
        queue_t *source_queue = queue_for_priority(old_priority);
        queue_t *buffer_queue = queue_create();
        if (buffer_queue == NULL) {
            interrupts_restore(flags);
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

    process->priority = target_priority;
    if (process->is_shell && process->priority < SCHEDULER_MAX_PRIORITY) {
        process->priority = SCHEDULER_MAX_PRIORITY;
    }
    process->priority_requested = process->priority;
    process->priority_fixed = 1;

    if (process->state == PROCESS_STATE_READY && process->priority != old_priority) {
        queue_push(queue_for_priority(process->priority), process);
    }

    if (process == scheduler.current) {
        process->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    }
    interrupts_restore(flags);

    return 0;
}
