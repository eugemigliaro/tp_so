#include <scheduler.h>

typedef struct scheduler_state {
    process_queue_t ready_queue;
    pcb_t *current;
    pcb_t *idle;
    int initialized;
    int need_reschedule;
    uint8_t base_quantum;
    scheduler_metrics_t metrics;
} scheduler_state_t;

static scheduler_state_t scheduler_state;

static void mark_running(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    pcb_set_state(pcb, PROCESS_STATE_RUNNING);
}

static void mark_ready(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    pcb_set_state(pcb, PROCESS_STATE_READY);
}

void scheduler_init(void) {
    process_queue_init(&scheduler_state.ready_queue);
    scheduler_state.current = NULL;
    scheduler_state.idle = NULL;
    scheduler_state.initialized = 1;
    scheduler_state.need_reschedule = 0;
    scheduler_state.base_quantum = SCHEDULER_DEFAULT_QUANTUM;
    scheduler_state.metrics.total_ticks = 0;
    scheduler_state.metrics.context_switches = 0;
}

int scheduler_is_initialized(void) {
    return scheduler_state.initialized;
}

void scheduler_set_idle_process(pcb_t *idle) {
    scheduler_state.idle = idle;
}

pcb_t *scheduler_current(void) {
    return scheduler_state.current;
}

void scheduler_set_current(pcb_t *pcb) {
    if (!scheduler_state.initialized) {
        return;
    }

    if (scheduler_state.current != pcb) {
        scheduler_state.metrics.context_switches++;
    }

    scheduler_state.current = pcb;
    scheduler_state.need_reschedule = 0;

    if (pcb != NULL) {
        mark_running(pcb);
    }
}

void scheduler_enqueue_ready(pcb_t *pcb) {
    if (!scheduler_state.initialized || pcb == NULL) {
        return;
    }
    mark_ready(pcb);
    process_queue_push(&scheduler_state.ready_queue, pcb);
}

void scheduler_enqueue_ready_front(pcb_t *pcb) {
    if (!scheduler_state.initialized || pcb == NULL) {
        return;
    }
    mark_ready(pcb);
    process_queue_push_front(&scheduler_state.ready_queue, pcb);
}

pcb_t *scheduler_pick_next(void) {
    if (!scheduler_state.initialized) {
        return NULL;
    }

    pcb_t *next = process_queue_pop(&scheduler_state.ready_queue);
    if (next == NULL) {
        next = scheduler_state.idle;
    }
    return next;
}

int scheduler_has_ready(void) {
    if (!scheduler_state.initialized) {
        return 0;
    }
    return !process_queue_is_empty(&scheduler_state.ready_queue);
}

void scheduler_on_tick(void) {
    if (!scheduler_state.initialized) {
        return;
    }
    scheduler_state.metrics.total_ticks++;
}

int scheduler_needs_reschedule(void) {
    if (!scheduler_state.initialized) {
        return 0;
    }
    return scheduler_state.need_reschedule;
}

void scheduler_ack_reschedule(void) {
    scheduler_state.need_reschedule = 0;
}

uint64_t *scheduler_handle_timer_interrupt(uint64_t *stack_frame) {
    (void)stack_frame;
    return stack_frame;
}

const scheduler_metrics_t *scheduler_get_metrics(void) {
    return &scheduler_state.metrics;
}
