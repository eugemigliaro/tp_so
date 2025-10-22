#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <process.h>

#define SCHEDULER_DEFAULT_QUANTUM 4

typedef struct scheduler_metrics {
    uint64_t total_ticks;
    uint64_t context_switches;
} scheduler_metrics_t;

void scheduler_init(void);
int scheduler_is_initialized(void);

void scheduler_set_idle_process(pcb_t *idle);

pcb_t *scheduler_current(void);
void scheduler_set_current(pcb_t *pcb);

void scheduler_enqueue_ready(pcb_t *pcb);
void scheduler_enqueue_ready_front(pcb_t *pcb);
pcb_t *scheduler_pick_next(void);
int scheduler_has_ready(void);

void scheduler_on_tick(void);
int scheduler_needs_reschedule(void);
void scheduler_ack_reschedule(void);
uint64_t *scheduler_handle_timer_interrupt(uint64_t *stack_frame);

const scheduler_metrics_t *scheduler_get_metrics(void);

#endif
