#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <collections/list.h>

#define PROCESS_PRIORITY_MIN 0
#define PROCESS_PRIORITY_MAX 7
#define PROCESS_PRIORITY_LEVELS (PROCESS_PRIORITY_MAX - PROCESS_PRIORITY_MIN + 1)
#define PROCESS_DEFAULT_PRIORITY 3
#define PROCESS_IDLE_PRIORITY PROCESS_PRIORITY_MAX
#define PROCESS_NAME_MAX_LEN 64

typedef struct process_queue process_queue_t;

typedef enum {
    PROCESS_STATE_NEW = 0,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_SLEEPING,
    PROCESS_STATE_ZOMBIE,
    PROCESS_STATE_EXITED
} process_state_t;

typedef enum {
    PROCESS_BACKGROUND = 0,
    PROCESS_FOREGROUND = 1
} process_foreground_t;

typedef struct cpu_context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} cpu_context_t;

typedef struct process_control_block {
    uint64_t pid;
    uint64_t ppid;
    const char *name;
    void *entry_point;
    void *stack_base;
    void *stack_top;
    process_state_t state;
    process_foreground_t foreground;
    uint8_t priority;
    uint64_t ticks_remaining;
    uint64_t quantum_ticks;
    cpu_context_t context;
    struct process_control_block *parent;
    process_queue_t *current_queue;
    list_t children;
    list_node_t sched_node;
    list_node_t sibling_node;
    uint64_t exit_code;
} process_control_block_t;

typedef process_control_block_t pcb_t;

struct process_queue {
    list_t levels[PROCESS_PRIORITY_LEVELS];
    size_t process_count;
};

void process_queue_init(process_queue_t *queue);
int process_queue_is_empty(const process_queue_t *queue);
size_t process_queue_size(const process_queue_t *queue);
void process_queue_push(process_queue_t *queue, pcb_t *pcb);
void process_queue_push_front(process_queue_t *queue, pcb_t *pcb);
pcb_t *process_queue_pop(process_queue_t *queue);
pcb_t *process_queue_pop_priority(process_queue_t *queue, uint8_t priority);
void process_queue_remove(process_queue_t *queue, pcb_t *pcb);
pcb_t *process_queue_peek(const process_queue_t *queue);
pcb_t *process_queue_peek_priority(const process_queue_t *queue, uint8_t priority);

uint8_t process_priority_clamp(uint8_t priority);
int process_priority_valid(uint8_t priority);

void pcb_init(pcb_t *pcb,
              uint64_t pid,
              const char *name,
              void *entry_point,
              void *stack_base,
              void *stack_top,
              uint8_t priority,
              process_foreground_t foreground);
void pcb_reset_context(pcb_t *pcb);
void pcb_set_priority(pcb_t *pcb, uint8_t priority);
void pcb_set_state(pcb_t *pcb, process_state_t state);
void pcb_attach_child(pcb_t *parent, pcb_t *child);
void pcb_detach_child(pcb_t *child);

#endif
