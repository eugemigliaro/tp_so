#include <process.h>
#include <memoryManager.h>
#include <strings.h>
#include <lib.h>
#include <scheduler.h>
#include <interrupts.h>
#include <sem.h>
#include <queueADT.h>

#define PID_TO_INDEX(pid) ((pid) - PROCESS_FIRST_PID)

static pcb_t *pcb_table[PROCESS_MAX_PROCESSES] = {0};
static int32_t running_pid = -1;    // Only one running process at a time (unicore)

void process_table_init(void) {
    running_pid = -1;
}

pcb_t *process_lookup(uint64_t pid) {
    if (pid < PROCESS_FIRST_PID) {
        return NULL;
    }
    uint64_t index = PID_TO_INDEX(pid);
    if (index >= PROCESS_MAX_PROCESSES) {
        return NULL;
    }
    return pcb_table[index];
}

bool process_register(pcb_t *pcb) {
    if (pcb == NULL) {
        return false;
    }
    if (pcb->pid < PROCESS_FIRST_PID) {
        return false;
    }
    uint64_t index = PID_TO_INDEX(pcb->pid);
    if (index >= PROCESS_MAX_PROCESSES) {
        return false;
    }
    if (pcb_table[index] != NULL) {
        return false; // PID already in use
    }
    pcb_table[index] = pcb;
    return true;
}

void process_unregister(uint64_t pid) {
    if (pid < PROCESS_FIRST_PID) {
        return;
    }
    uint64_t index = PID_TO_INDEX(pid);
    if (index >= PROCESS_MAX_PROCESSES) {
        return;
    }
    if (pcb_table[index] != NULL && running_pid == (int32_t)pid) {
        running_pid = -1;
    }
    pcb_table[index] = NULL;
}

void process_set_running(pcb_t *pcb) {
    if (pcb == NULL) {
        running_pid = -1;
        return;
    }
    running_pid = (int32_t)pcb->pid;
}

bool process_block(pcb_t *pcb) {
    if (pcb == NULL) {
        return false;
    }

    pcb->state = PROCESS_STATE_BLOCKED;

    return true;
}

bool process_unblock(pcb_t *pcb) {
    if (pcb == NULL) {
        return false;
    }

    if (pcb->state != PROCESS_STATE_BLOCKED) {
        return false;
    }

    scheduler_add_ready(pcb);
    return true;
}

void process_free_memory(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }

    if (pcb->argv != NULL) {
        for (int i = 0; i < pcb->argc; i++) {
            if (pcb->argv[i] != NULL) {
                mem_free(pcb->argv[i]);
                pcb->argv[i] = NULL;
            }
        }
        mem_free(pcb->argv);
        pcb->argv = NULL;
    }

    if (pcb->stack_base != NULL) {
        mem_free(pcb->stack_base);
        pcb->stack_base = NULL;
    }

    if (pcb->exit_sem != NULL) {
        sem_destroy(pcb->exit_sem);
        mem_free(pcb->exit_sem);
        pcb->exit_sem = NULL;
    }

    mem_free(pcb);
}

bool process_exit(pcb_t *pcb) {
    if (pcb == NULL) {
        return false;
    }
    pcb->state = PROCESS_STATE_TERMINATED;

    sem_post(pcb->exit_sem);

    _force_scheduler_interrupt();

    return true;
}

int32_t get_pid(void) {
    if (running_pid < PROCESS_FIRST_PID) {
		return -1;
	}
	if (running_pid >= (int32_t)(PROCESS_FIRST_PID + PROCESS_MAX_PROCESSES)) {
		return -1;
	}
	return running_pid;
}

static uint64_t allocate_pid(void) {
    for(uint64_t i = 0; i < PROCESS_MAX_PROCESSES; i++) {
        if (pcb_table[i] == NULL) {
            return PROCESS_FIRST_PID + i;
        }
    }

    return 0; // No available PID
}

static const char *process_state_to_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_READY:
            return "ready";
        case PROCESS_STATE_RUNNING:
            return "running";
        case PROCESS_STATE_YIELD:
            return "yield";
        case PROCESS_STATE_BLOCKED:
            return "blocked";
        case PROCESS_STATE_TERMINATED:
            return "terminated";
        default:
            return "unknown";
    }
}

pcb_t *createProcess(int argc, char **argv, uint64_t ppid, uint8_t priority, uint8_t foreground, void *entry_point) {
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

    memset(pcb, 0, sizeof(pcb_t));
    pcb->pid = pid;
    pcb->ppid = ppid;
    pcb->priority = priority;
    pcb->foreground = foreground;
    pcb->state = PROCESS_STATE_READY;
    pcb->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    pcb->last_quantum_ticks = 0;
    pcb->argc = argc;
    pcb->children = NULL;

    void *stack_base = mem_alloc(PROCESS_STACK_SIZE);
    if (stack_base == NULL) {
        process_free_memory(pcb);
        return NULL;
    }
    pcb->stack_base = stack_base;

    void *stack_top = (uint8_t *)stack_base + PROCESS_STACK_SIZE - sizeof(uint64_t);
    uint64_t prepared_rsp = (uint64_t)stackInit(stack_top, (void *)entry_point, argc, argv);
    pcb->context.rsp = prepared_rsp;

    pcb->argv = (char **)mem_alloc(sizeof(char *) * pcb->argc);
    if (pcb->argv == NULL) {
        process_free_memory(pcb);
        return NULL;
    }
    memset(pcb->argv, 0, sizeof(char *) * pcb->argc);

    for (int i = 0; i < pcb->argc; i++) {
        pcb->argv[i] = (char *)mem_alloc(sizeof(char) * (strlen(argv[i]) + 1));
        if (pcb->argv[i] == NULL) {
            process_free_memory(pcb);
            return NULL;
        }
        strcpy(pcb->argv[i], argv[i]);
        pcb->argv[i][strlen(argv[i])] = '\0';
    }

    if (pcb->argc > 0) {
        pcb->name = pcb->argv[0]; // Set process name to first argument
    }

    pcb->exit_sem = sem_create();
    if (pcb->exit_sem == NULL) {
        process_free_memory(pcb);
        return NULL;
    }

    char sem_name[16] = "process";
    sem_name[7] = '0' + (char)((pid / 100) % 10);
    sem_name[8] = '0' + (char)((pid / 10) % 10);
    sem_name[9] = '0' + (char)(pid % 10);
    sem_name[10] = '\0';

    sem_init(pcb->exit_sem, sem_name, 0);
    if (pcb->exit_sem->name == NULL || pcb->exit_sem->waiting_processes == NULL) {
        process_free_memory(pcb);
        return NULL;
    }
    if (ppid >= PROCESS_FIRST_PID) {
        pcb_t *parent = process_lookup(ppid);
        if(parent == NULL) {
            process_free_memory(pcb);
            return NULL;
        }
        add_child(parent, pcb);
    }   

    if (!process_register(pcb)) {
        process_free_memory(pcb);
        return NULL;
    }

    return pcb;
}

int32_t print_process_list(void) {
    const int32_t current_pid = running_pid;
    int32_t count = 0;

    print("=== Process list ===\n");

    for (uint64_t i = 0; i < PROCESS_MAX_PROCESSES; ++i) {
        pcb_t *pcb = pcb_table[i];
        if (pcb == NULL) {
            continue;
        }

        const char *name = (pcb->name != NULL) ? pcb->name : "<unnamed>";

        print("PID: ");
        printDec(pcb->pid);
        print(" | Name: ");
        print(name);
        print(" | PPID: ");
        printDec(pcb->ppid);
        newLine();

        print("    State: ");
        print(process_state_to_string(pcb->state));
        print(" | Priority: ");
        printDec(pcb->priority);
        print(" | Foreground: ");
        print(pcb->foreground ? "yes" : "no");
        print(" | Running: ");
        print(pcb->pid == (uint64_t)current_pid ? "yes" : "no");
        newLine();

        print("    Stack base: 0x");
        printHex((uint64_t)pcb->stack_base);
        print(" | Stack ptr: 0x");
        printHex(pcb->context.rsp);
        newLine();

        print("    Remaining quantum: ");
        printDec(pcb->remaining_quantum);
        print(" | Last quantum ticks: ");
        printDec(pcb->last_quantum_ticks);
        newLine();

        count++;
        newLine();
    }

    if (count == 0) {
        print("No processes found.\n");
    } else {
        print("Total processes: ");
        printDec((uint64_t)count);
        newLine();
    }

    return count;
}

static bool is_child(pcb_t *parent, pcb_t *process) {
    if (parent == NULL || process == NULL) {
        return false;
    }
    return process->ppid == parent->pid;
}

static int32_t reap_child_process(pcb_t *process) {
    if (process == NULL) {
        return -1;
    }

    sem_wait(process->exit_sem);

    process_unregister(process->pid);
    process_free_memory(process);
    return 0;
}

int32_t process_wait_pid(uint64_t pid) { 

    _cli();
    pcb_t * current = scheduler_current();
    _sti();

    if (current == NULL) {
        return -1;
    }

    pcb_t *child = process_lookup(pid);

    if (child == NULL) {
        return -1;
    }

    if (!is_child(current, child)) {
        return -1;
    }

    bool removed = false;
    if (current->children != NULL) {
        removed = queue_remove(current->children, child);
    }

    if (!removed) {
        return -1;
    }

    return reap_child_process(child);

}

int32_t process_wait_children(void) {

    _cli();
    pcb_t *current = scheduler_current();
    _sti();

    if(current == NULL) {
        return -1;
    }

    if(current->children == NULL || queue_is_empty(current->children)) {
        return 0;
    }
    pcb_t *child = (pcb_t *)queue_pop(current->children);
    while (child != NULL) {

        if (!is_child(current, child)) {
            return -1;
        }

        if (reap_child_process(child) != 0) {
            return -1;
        }

        child = (pcb_t *)queue_pop(current->children);
    }

    return 0;
}

void add_child(pcb_t *parent, pcb_t *child) {
    if(parent == NULL || child == NULL) {
        return;
    }

    if(parent->children == NULL){
        parent->children = queue_create();
    }
    if(parent->children == NULL){
        return;
    }

    queue_push(parent->children, (void *)child);
}
