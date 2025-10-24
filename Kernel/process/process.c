#include <process.h>
#include <memoryManager.h>
#include <strings.h>
#include <lib.h>
#include <scheduler.h>

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

pcb_t *createProcess(int argc, char **argv, uint64_t ppid, uint8_t priority, void (*entry_point)(void)) {
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

    void *stack_top = (uint8_t *)stack_base + PROCESS_STACK_SIZE - sizeof(uint64_t);
    uint64_t prepared_rsp = (uint64_t)stackInit(stack_top, (void *)entry_point, argc, argv);

    pcb->pid = pid;
    pcb->ppid = ppid;
    pcb->priority = priority;
    pcb->state = PROCESS_STATE_READY;
    pcb->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    pcb->last_quantum_ticks = 0;
    pcb->context.rsp = prepared_rsp;
    pcb->stack_base = stack_base;
    pcb->argc = argc;
    pcb->argv = (char **)mem_alloc(sizeof(char *) * pcb->argc);

	if (pcb->argv == NULL) {
		// @todo: ENOMEM
        mem_free(pcb);
		return NULL;
	}
	for (int i = 0; i < pcb->argc; i++) {
		// @todo: ENOMEM
		pcb->argv[i] = (char *)mem_alloc(sizeof(char) * (strlen(argv[i]) + 1));
		if (pcb->argv[i] == NULL) {
			for (int j = 0; j < i; j++) {
				mem_free(pcb->argv[j]);
			}
			mem_free(pcb->argv);
			mem_free(pcb);
			return NULL;
		}
		strcpy(pcb->argv[i], argv[i]);
        pcb->argv[i][strlen(argv[i])] = '\0';
	}

    pcb->name = pcb->argv[0]; // Set process name to first argument

    if (!process_register(pcb)) {
        mem_free(pcb);
        mem_free(stack_base);
        return NULL;
    }

    return pcb;
}
