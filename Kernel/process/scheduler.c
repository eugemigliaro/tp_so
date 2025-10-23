#include <stddef.h>
#include <scheduler.h>
#include <queueADT.h>
#include <interrupts.h>

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

void scheduler_init(void) {
    for (size_t i = 0; i < SCHEDULER_PRIORITY_LEVELS; i++) {
        queue_t *queue = queue_create();
        scheduler.ready_queues[i] = queue;
    }

    scheduler.idle = createProcess("idle", 0, SCHEDULER_MIN_PRIORITY, idle_process_entry);
}

void scheduler_add_ready(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    pcb->state = PROCESS_STATE_READY;
    queue_push(queue_for_priority(pcb->priority), pcb);
}

pcb_t *scheduler_current(void) {
    return scheduler.current;
}


void *schedule_tick(void *current_rsp) {
    scheduler.metrics.total_ticks++;

    pcb_t *running = scheduler.current;
    if (running != NULL) {
        running->context.rsp = (uint64_t)current_rsp;
        scheduler_add_ready(running);
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
            scheduler.metrics.context_switches++;
            return (void *)next->context.rsp;
        }
    }

    if (scheduler.idle != NULL) {
        scheduler.idle->context.rsp = (uint64_t)current_rsp;
        scheduler.idle->state = PROCESS_STATE_RUNNING;
        scheduler.current = scheduler.idle;
        return (void *)scheduler.idle->context.rsp;
    }

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
