#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PROCESS_MAX_PROCESSES 32
#define PROCESS_STACK_SIZE (4096 * 4)
#define PROCESS_NAME_MAX 32

typedef enum process_state {
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} process_state_t;

typedef struct context{
    uint64_t rsp; // I think it is enough to store only this, since the rest are in the stack
} context_t;

typedef struct pcb {
    uint64_t pid;
    uint64_t ppid;
    char name[PROCESS_NAME_MAX];
    process_state_t state;
    uint8_t priority;
    context_t context; //should this be a pointer?
    uint8_t last_ticks_used;
    uint8_t last_quantum;
    void *stack_base;
} pcb_t;

void process_table_init(void);
pcb_t *process_lookup(uint64_t pid);
bool process_register(pcb_t *pcb);
void process_unregister(uint64_t pid);

pcb_t *createProcess(const char *name, uint64_t ppid, uint8_t priority, void (*entry_point)(void));

#endif
