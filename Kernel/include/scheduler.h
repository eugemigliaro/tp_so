#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <process.h>
#include <stdbool.h>

#define SCHEDULER_DEFAULT_QUANTUM 4
#define SCHEDULER_MAX_PRIORITY 0
#define SCHEDULER_MIN_PRIORITY 5
#define SCHEDULER_PRIORITY_LEVELS (SCHEDULER_MIN_PRIORITY - SCHEDULER_MAX_PRIORITY + 1)
#define SCHEDULER_AGING_THRESHOLD 32

typedef struct scheduler_metrics {
    uint64_t total_ticks;
    uint64_t context_switches;
} scheduler_metrics_t;

typedef void (*scheduler_iter_cb)(process_t *process, void *context);

void scheduler_init(void);
void scheduler_add_ready(process_t *process);
bool scheduler_remove_ready(process_t *process);
process_t *scheduler_current(void);
void scheduler_clear_current(process_t *process);
void *schedule_tick(void *current_rsp);
const scheduler_metrics_t *scheduler_get_metrics(void);

void scheduler_for_each_ready(scheduler_iter_cb callback, void *context);
int32_t scheduler_set_process_priority(uint32_t pid, uint8_t priority);
bool scheduler_has_ready_other(process_t *exclude);

#endif
