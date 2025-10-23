#include <process.h>
#include <memoryManager.h>
#include <strings.h>
#include <lib.h>

static uint64_t next_pid = 1;
static pcb_t *pcb_table[PROCESS_MAX_PROCESSES] = {0};

void process_table_init(void) {
    for (size_t i = 0; i < PROCESS_MAX_PROCESSES; ++i) {
        pcb_table[i] = NULL;
    }
    next_pid = 1;
}

pcb_t *process_lookup(uint64_t pid) {
    if (pid == 0 || pid >= PROCESS_MAX_PROCESSES) {
        return NULL;
    }
    return pcb_table[pid];
}

bool process_register(pcb_t *pcb) {
    if (pcb == NULL) {
        return false;
    }
    if (pcb->pid == 0 || pcb->pid >= PROCESS_MAX_PROCESSES) {
        return false;
    }
    pcb_table[pcb->pid] = pcb;
    return true;
}

void process_unregister(uint64_t pid) {
    if (pid == 0 || pid >= PROCESS_MAX_PROCESSES) {
        return;
    }
    pcb_table[pid] = NULL;
}

static uint64_t allocate_pid(void) {
    if (next_pid >= PROCESS_MAX_PROCESSES) {
        return 0;
    }
    return next_pid++;
}

pcb_t *createProcess(const char *name, uint64_t ppid, uint8_t priority, void (*entry_point)(void)) {
    if (entry_point == NULL) {
        return NULL;
    }

    uint64_t pid = allocate_pid();
    if (pid == 0) {
        return NULL;
    }

    pcb_t *pcb = mem_alloc(sizeof(pcb_t));
    if (pcb == NULL) {
        return NULL;
    }

    void *stack_base = mem_alloc(PROCESS_STACK_SIZE);
    if (stack_base == NULL) {
        mem_free(pcb);
        return NULL;
    }

    void *stack_top = (uint8_t *)stack_base + PROCESS_STACK_SIZE;
    uint64_t prepared_rsp = stackInit(stack_top, 0, stack_top, entry_point);

    pcb->pid = pid;
    pcb->ppid = ppid;
    pcb->priority = priority;
    pcb->state = PROCESS_STATE_READY;
    pcb->last_ticks_used = 0;
    pcb->last_quantum = 0;
    pcb->context.rsp = prepared_rsp;
    pcb->stack_base = stack_base;

    if (name == NULL) {
        pcb->name[0] = '\0';
    } else {
        strcpy(pcb->name, name);
    }

    if (!process_register(pcb)) {
        mem_free(pcb);
        mem_free(stack_base);
        return NULL;
    }

    return pcb;
}
