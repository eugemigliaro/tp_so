#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PROCESS_FIRST_PID 1
#define PROCESS_MAX_PROCESSES 32
#define PROCESS_STACK_SIZE (4096 * 4)

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
    char *name;
    int argc;
    char **argv;
    process_state_t state;
    uint8_t priority;
    uint8_t foreground;
    context_t context; //should this be a pointer?
    uint8_t remaining_quantum;
    uint8_t last_quantum_ticks;
    void *stack_base;
} pcb_t;

pcb_t *process_lookup(uint64_t pid);
bool process_register(pcb_t *pcb);
void process_unregister(uint64_t pid);
void process_table_init(void);
uint64_t get_next_pid(void);
int32_t get_pid(void);
void process_set_running(pcb_t *pcb);
bool process_block(pcb_t *pcb);
bool process_unblock(pcb_t *pcb);
bool process_remove(pcb_t *pcb);
void process_free_memory(pcb_t *pcb);

pcb_t *createProcess(int argc, char **argv, uint64_t ppid, uint8_t priority, uint8_t foreground, void (*entry_point)(void));
int32_t print_process_list(void);

#endif
