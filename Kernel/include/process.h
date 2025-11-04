#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sem.h>
#include <queueADT.h>

#define PROCESS_FIRST_PID 1
#define PROCESS_MAX_PROCESSES 32
#define PROCESS_STACK_SIZE (4096 * 4)
#define PROCESS_MAX_CHILDREN PROCESS_MAX_PROCESSES

typedef enum process_state {
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_YIELD,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} process_state_t;

typedef struct context{
    uint64_t rsp; // I think it is enough to store only this, since the rest are in the stack
} context_t;

typedef struct process {
    uint32_t pid;
    uint32_t ppid;
    char *name;
    int argc;
    char **argv;
    uint8_t fd_targets[3];
    process_state_t state;
    uint8_t priority;
    context_t context; //should this be a pointer?
    uint8_t remaining_quantum;
    uint8_t last_quantum_ticks;
    void *stack_base;
    sem_t *exit_sem;
    queue_t *children;
} process_t;

process_t *process_lookup(uint32_t pid);
bool process_register(process_t *process);
void process_unregister(uint32_t pid);
void process_table_init(void);
int32_t get_pid(void);
void process_set_running(process_t *process);
bool process_block(process_t *process);
bool process_unblock(process_t *process);
bool process_exit(process_t *process);
void process_free_memory(process_t *process);
int32_t process_wait_pid(uint32_t pid);
int32_t process_wait_children(void);

process_t *createProcess(int argc, char **argv, uint32_t ppid, uint8_t priority, uint8_t foreground, void *entry_point);
int32_t add_first_process(void);
int32_t print_process_list(void);   

void add_child(process_t *parent, process_t *child);

int get_foreground_process_pid(void);
int give_foreground_to(uint32_t target_pid);


#endif
