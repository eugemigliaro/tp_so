// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <process.h>
#include <memoryManager.h>
#include <strings.h>
#include <lib.h>
#include <scheduler.h>
#include <interrupts.h>
#include <sem.h>
#include <pipes.h>
#include <queueADT.h>

#define PID_TO_INDEX(pid) ((pid) - PROCESS_FIRST_PID)
#define INIT_PROCESS_NAME "init"
#define SHELL_PROCESS_NAME "shell"
#define SHELL_PROCESS_ENTRY ((void *)0x400000)
static void init_first_process_entry(int argc, char **argv);
static int32_t reap_child_process(process_t *process);
static int shell_created = 0;
static void adopt_orphan_children(process_t *process);

static process_t *ready_scan_self = NULL;
static uint8_t ready_scan_found = 0;
static void mark_ready_process(process_t *process, void *context) {
    (void)context;
    if (process != NULL && process != ready_scan_self) {
        ready_scan_found = 1;
    }
}

typedef struct pcb {
    process_t *processes[PROCESS_MAX_PROCESSES];
    size_t process_count;
    int32_t running_pid;
    int32_t foreground_pid;
    int32_t init_pid;
    int32_t main_shell_pid;
} pcb_t;

static pcb_t *pcb = NULL;

void process_table_init(void) {
    if (pcb != NULL) {
        return;
    }

    pcb = (pcb_t *)mem_alloc(sizeof(pcb_t));
    if (pcb == NULL) {
        return;
    }

    memset(pcb, 0, sizeof(pcb_t));
    pcb->running_pid = -1;
    pcb->foreground_pid = -1;
    pcb->init_pid = -1;
    pcb->main_shell_pid = -1;
}

process_t *process_lookup(uint32_t pid) {
    if (pcb == NULL || pid < PROCESS_FIRST_PID) {
        return NULL;
    }

    uint32_t index = PID_TO_INDEX(pid);
    if (index >= PROCESS_MAX_PROCESSES) {
        return NULL;
    }

    return pcb->processes[index];
}

bool process_register(process_t *process) {
    if (pcb == NULL || process == NULL) {
        return false;
    }

    if (process->pid < PROCESS_FIRST_PID) {
        return false;
    }

    uint32_t index = PID_TO_INDEX(process->pid);
    if (index >= PROCESS_MAX_PROCESSES) {
        return false;
    }

    if (pcb->processes[index] != NULL) {
        return false;
    }

    pcb->processes[index] = process;
    pcb->process_count++;
    return true;
}

void process_unregister(uint32_t pid) {
    if (pcb == NULL || pid < PROCESS_FIRST_PID) {
        return;
    }

    uint32_t index = PID_TO_INDEX(pid);
    if (index >= PROCESS_MAX_PROCESSES) {
        return;
    }

    process_t *process = pcb->processes[index];
    if (process == NULL) {
        return;
    }

    if (pcb->running_pid == (int32_t)pid) {
        pcb->running_pid = -1;
    }

    pcb->processes[index] = NULL;
    if (pcb->process_count > 0) {
        pcb->process_count--;
    }
}

void process_set_running(process_t *process) {
    if (pcb == NULL) {
        return;
    }

    if (process == NULL) {
        pcb->running_pid = -1;
        return;
    }

    pcb->running_pid = (int32_t)process->pid;
}

bool process_block(process_t *process) {
    if (process == NULL) {
        return false;
    }

    if (process->state == PROCESS_STATE_BLOCKED) {
        return false;
    }

    if (process->state == PROCESS_STATE_READY) {
        scheduler_remove_ready(process);
    }

    process->state = PROCESS_STATE_BLOCKED;

    return true;
}

bool process_unblock(process_t *process) {
    if (process == NULL) {
        return false;
    }

    if (process->state != PROCESS_STATE_BLOCKED) {
        return false;
    }

    scheduler_add_ready(process);
    return true;
}

void process_free_memory(process_t *process) {
    if (process == NULL) {
        return;
    }

    if (process->argv != NULL) {
        for (int i = 0; i < process->argc; i++) {
            if (process->argv[i] != NULL) {
                mem_free(process->argv[i]);
                process->argv[i] = NULL;
            }
        }
        mem_free(process->argv);
        process->argv = NULL;
    }

    if (process->stack_base != NULL) {
        mem_free(process->stack_base);
        process->stack_base = NULL;
    }

    if (process->exit_sem != NULL) {
        sem_destroy(process->exit_sem);
        mem_free(process->exit_sem);
        process->exit_sem = NULL;
    }

    if (process->children != NULL) {
        queue_destroy(process->children, NULL);
        process->children = NULL;
    }

    mem_free(process);
}

bool process_exit(process_t *process) {
    if (process == NULL) {
        return false;
    }
    
    uint8_t stdin_id = process->fd_targets[STDIN];
    uint8_t stdout_id = process->fd_targets[STDOUT];
    uint8_t stderr_id = process->fd_targets[STDERR];
    
    uint64_t flags = interrupts_save_and_disable();
    
    if (process->state == PROCESS_STATE_READY) {
        scheduler_remove_ready(process);
    }
    
    process->state = PROCESS_STATE_TERMINATED;

    adopt_orphan_children(process);

    if (pcb != NULL && pcb->foreground_pid == (int32_t)process->pid) {
        pcb->foreground_pid = (int32_t)process->ppid;
    }
    
    int should_force_switch = (pcb != NULL && pcb->running_pid == (int32_t)process->pid);
    
    interrupts_restore(flags);

    unattach_from_pipe(stdin_id, (int)process->pid);
    unattach_from_pipe(stdout_id, (int)process->pid);
    unattach_from_pipe(stderr_id, (int)process->pid);

    close_pipe(stdin_id);
    close_pipe(stdout_id);
    close_pipe(stderr_id);
    
    sem_post(process->exit_sem);
    
    if (should_force_switch) {
        _force_scheduler_interrupt();
    }
    
    return true;
}

void process_yield(void) {
    process_t *current = scheduler_current();
    if (current == NULL) {
        return;
    }

    current->remaining_quantum = 0;
    _force_scheduler_interrupt();
}

int32_t get_pid(void) {
    if (pcb == NULL) {
        return -1;
    }

    int32_t current = pcb->running_pid;
    if (current < PROCESS_FIRST_PID) {
        return -1;
    }

    if (current >= (int32_t)(PROCESS_FIRST_PID + PROCESS_MAX_PROCESSES)) {
        return -1;
    }

    return current;
}

static uint32_t allocate_pid(void) {
    if (pcb == NULL) {
        return 0;
    }

    for(uint32_t i = 0; i < PROCESS_MAX_PROCESSES; i++) {
        if (pcb->processes[i] == NULL) {
            return PROCESS_FIRST_PID + i;
        }
    }

    return 0;
}

static const char *process_state_to_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_READY:
            return "ready";
        case PROCESS_STATE_RUNNING:
            return "running";
        case PROCESS_STATE_BLOCKED:
            return "blocked";
        case PROCESS_STATE_TERMINATED:
            return "terminated";
        default:
            return "unknown";
    }
}

static void process_entry_wrapper(int argc, char **argv) {
    process_t *current = scheduler_current();
    if (current == NULL || current->user_entry_point == NULL) {
        process_exit(current);
    }
    
    int (*user_func)(int, char**) = (int (*)(int, char**))current->user_entry_point;
    int exit_code = user_func(argc, argv);
    
    (void)exit_code;
    
    process_exit(current);
    
    while(1) {
        process_yield();
    }
}

process_t *createProcess(int argc, char **argv, uint32_t ppid, uint8_t priority, uint8_t foreground, void *entry_point) {
    if (entry_point == NULL) {
        return NULL;
    }

    uint32_t pid = allocate_pid();
    if (pid == 0) {
        return NULL;
    }

    process_t *process = mem_alloc(sizeof(process_t));
    if (process == NULL) {
        return NULL;
    }

    memset(process, 0, sizeof(process_t));
    process->pid = pid;
    process->ppid = ppid;
    process->priority = priority;
    process->priority_requested = priority;
    process->priority_fixed = 0;
    process->is_shell = 0;
    process->state = PROCESS_STATE_READY;
    process->remaining_quantum = SCHEDULER_DEFAULT_QUANTUM;
    process->last_quantum_ticks = 0;
    process->argc = argc;
    process->children = NULL;
    process->user_entry_point = entry_point;

    process_t *parent = NULL;
    uint8_t stdin_target = STDIN;
    uint8_t stdout_target = STDOUT;
    uint8_t stderr_target = STDERR;
    if (ppid >= PROCESS_FIRST_PID) {
        parent = process_lookup(ppid);
        if (parent == NULL) {
            process_free_memory(process);
            return NULL;
        }
        stdin_target = parent->fd_targets[STDIN];
        stdout_target = parent->fd_targets[STDOUT];
        stderr_target = parent->fd_targets[STDERR];
    }
    process->fd_targets[STDIN] = stdin_target;
    process->fd_targets[STDOUT] = stdout_target;
    process->fd_targets[STDERR] = stderr_target;

    size_t argv_count = (size_t)process->argc + 1u;
    process->argv = (char **)mem_alloc(sizeof(char *) * argv_count);
    if (process->argv == NULL) {
        process_free_memory(process);
        return NULL;
    }
    memset(process->argv, 0, sizeof(char *) * argv_count);

    for (int i = 0; i < process->argc; i++) {
        size_t arg_len = strlen(argv[i]);
        process->argv[i] = (char *)mem_alloc(sizeof(char) * (arg_len + 1));
        if (process->argv[i] == NULL) {
            process_free_memory(process);
            return NULL;
        }
        strcpy(process->argv[i], argv[i]);
        process->argv[i][arg_len] = '\0';
    }
    process->argv[process->argc] = NULL;

    if (process->argc > 0) {
        process->name = process->argv[0];
    }

    void *stack_base = mem_alloc(PROCESS_STACK_SIZE);
    if (stack_base == NULL) {
        process_free_memory(process);
        return NULL;
    }
    process->stack_base = stack_base;

    void *stack_top = (uint8_t *)stack_base + PROCESS_STACK_SIZE - sizeof(uint64_t);
    uint64_t prepared_rsp =
        (uint64_t)stackInit(stack_top, (void *)process_entry_wrapper, process->argc, process->argv);
    process->context.rsp = prepared_rsp;

    process->exit_sem = sem_create();
    if (process->exit_sem == NULL) {
        process_free_memory(process);
        return NULL;
    }

    char sem_name[16] = "process";
    uint32_t pid_digits = pid;
    for (int idx = 9; idx >= 7; idx--) {
        sem_name[idx] = '0' + (char)(pid_digits % 10);
        pid_digits /= 10;
    }
    sem_name[10] = '\0';

    sem_init(process->exit_sem, sem_name, 0);
    if (process->exit_sem->name == NULL || process->exit_sem->waiting_processes == NULL) {
        process_free_memory(process);
        return NULL;
    }

    bool stdin_attached = false;
    bool stdout_attached = false;
    bool stderr_attached = false;

    if (attach_to_pipe(stdin_target) == 0) {
        stdin_attached = true;
    } else if (ppid >= PROCESS_FIRST_PID) {
        process_free_memory(process);
        return NULL;
    }
    if (attach_to_pipe(stdout_target) == 0) {
        stdout_attached = true;
    } else if (ppid >= PROCESS_FIRST_PID) {
        if (stdin_attached) {
            unattach_from_pipe(stdin_target, (int)process->pid);
        }
        process_free_memory(process);
        return NULL;
    }

    if (attach_to_pipe(stderr_target) == 0) {
        stderr_attached = true;
    } else if (ppid >= PROCESS_FIRST_PID) {
        if (stdout_attached) {
            unattach_from_pipe(stdout_target, (int)process->pid);
        }
        if (stdin_attached) {
            unattach_from_pipe(stdin_target, (int)process->pid);
        }
        process_free_memory(process);
        return NULL;
    }

    if (!process_register(process)) {
        if (stderr_attached) {
            unattach_from_pipe(stderr_target, (int)process->pid);
        }
        if (stdout_attached) {
            unattach_from_pipe(stdout_target, (int)process->pid);
        }
        if (stdin_attached) {
            unattach_from_pipe(stdin_target, (int)process->pid);
        }
        process_free_memory(process);
        return NULL;
    }

    if (parent != NULL) {
        add_child(parent, process);
    }

    if (foreground && pcb != NULL) {
        pcb->foreground_pid = (int32_t)process->pid;
    }

    return process;
}

static void init_first_process_entry(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    if (!shell_created) {
        process_t *self = scheduler_current();
        if (self != NULL) {
            char *shell_argv[] = {SHELL_PROCESS_NAME, NULL};
            process_t *shell =
                createProcess(1, shell_argv, self->pid, SCHEDULER_MAX_PRIORITY, 1, SHELL_PROCESS_ENTRY);
            if (shell != NULL) {
                shell->is_shell = 1;
                shell_created = 1;
                if (pcb != NULL) {
                    pcb->main_shell_pid = (int32_t)shell->pid;
                }
                scheduler_add_ready(shell);
            }
        }
    }

    while (1) {
        process_t *self = scheduler_current();
        if (self != NULL && self->children != NULL && !queue_is_empty(self->children)) {
            size_t children = queue_size(self->children);
            for (size_t i = 0; i < children; i++) {
                process_t *child = (process_t *)queue_pop(self->children);
                if (child == NULL) {
                    continue;
                }
                if (child->state == PROCESS_STATE_TERMINATED) {
                    int was_main_shell = (pcb != NULL) && ((int32_t)child->pid == pcb->main_shell_pid);
                    reap_child_process(child);
                    if (was_main_shell) {
                        char *shell_argv[] = {SHELL_PROCESS_NAME, NULL};
                        process_t *new_shell =
                            createProcess(1, shell_argv, self->pid, SCHEDULER_MAX_PRIORITY, 1, SHELL_PROCESS_ENTRY);
                        if (new_shell != NULL) {
                            new_shell->is_shell = 1;
                            scheduler_add_ready(new_shell);
                            pcb->main_shell_pid = (int32_t)new_shell->pid;
                        }
                    }
                } else {
                    queue_push(self->children, child);
                }
            }
        }

        ready_scan_self = self;
        ready_scan_found = 0;
        scheduler_for_each_ready(mark_ready_process, NULL);
        if (ready_scan_found) {
            process_yield();
        } 
    }
}
int32_t add_first_process(void) {
    int std_in = open_pipe();
    if (std_in < 0) {
        return -1;
    }
    if (std_in != STDIN) {
        close_pipe((uint8_t)std_in);
        return -1;
    }
    if (attach_to_pipe((uint8_t)std_in) != 0) {
        close_pipe((uint8_t)std_in);
        return -1;
    }
    int std_out = open_pipe();
    if (std_out < 0) {
        close_pipe((uint8_t)std_in);
        return -1;
    }
    if (std_out != STDOUT) {
        close_pipe((uint8_t)std_out);
        close_pipe((uint8_t)std_in);
        return -1;
    }
    if (attach_to_pipe((uint8_t)std_out) != 0) {
        close_pipe((uint8_t)std_out);
        close_pipe((uint8_t)std_in);
        return -1;
    }
    int std_err = open_pipe();
    if (std_err < 0) {
        close_pipe((uint8_t)std_out);
        close_pipe((uint8_t)std_in);
        return -1;
    }
    if (std_err != STDERR) {
        close_pipe((uint8_t)std_err);
        close_pipe((uint8_t)std_out);
        close_pipe((uint8_t)std_in);
        return -1;
    }
    if (attach_to_pipe((uint8_t)std_err) != 0) {
        close_pipe((uint8_t)std_err);
        close_pipe((uint8_t)std_out);
        close_pipe((uint8_t)std_in);
        return -1;
    }
    char *argv[] = {INIT_PROCESS_NAME, NULL};
    process_t *init_process =
        createProcess(1, argv, 0, SCHEDULER_MAX_PRIORITY, 0, (void *)init_first_process_entry);
    if (init_process == NULL) {
        close_pipe((uint8_t)std_err);
        close_pipe((uint8_t)std_out);
        close_pipe((uint8_t)std_in);
        return -1;
    }
    
    if (pcb != NULL) {
        pcb->init_pid = (int32_t)init_process->pid;
    }
    
    scheduler_add_ready(init_process);
    return (int32_t)init_process->pid;
}

int32_t print_process_list(void) {
    if (pcb == NULL) {
        print("No processes found.\n");
        return 0;
    }

    const int32_t current_pid = pcb->running_pid;
    int32_t count = 0;

    print("=== Process list ===\n");

    for (uint32_t i = 0; i < PROCESS_MAX_PROCESSES; ++i) {
        process_t *process = pcb->processes[i];
        if (process == NULL) {
            continue;
        }

        const char *name = (process->name != NULL) ? process->name : "<unnamed>";

        print("PID: ");
        printDec(process->pid);
        print(" | Name: ");
        print(name);
        print(" | PPID: ");
        printDec(process->ppid);
        newLine();

        print("    State: ");
        print(process_state_to_string(process->state));
        print(" | Priority: ");
        printDec(process->priority);
        print(" | Foreground: ");
        print(pcb->foreground_pid == (int32_t)process->pid ? "yes" : "no");
        print(" | Running: ");
        print(process->pid == (uint32_t)current_pid ? "yes" : "no");
        newLine();

        print("    Stack base: 0x");
        printHex((uint64_t)process->stack_base);
        print(" | Stack ptr: 0x");
        printHex(process->context.rsp);
        newLine();

        print("    Remaining quantum: ");
        printDec(process->remaining_quantum);
        print(" | Last quantum ticks: ");
        printDec(process->last_quantum_ticks);
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

static bool is_child(process_t *parent, process_t *process) {
    if (parent == NULL || process == NULL) {
        return false;
    }
    return process->ppid == parent->pid;
}

static int32_t reap_child_process(process_t *process) {
    if (process == NULL) {
        return -1;
    }

    sem_wait(process->exit_sem);

    process_unregister(process->pid);
    process_free_memory(process);
    return 0;
}

static void adopt_orphan_children(process_t *process) {
    if (process == NULL || process->children == NULL) {
        return;
    }

    if (pcb != NULL && (int32_t)process->pid == pcb->init_pid) {
        return;
    }

    uint64_t flags = interrupts_save_and_disable();
    
    process_t *init_process = NULL;
    if (pcb != NULL && pcb->init_pid >= 0) {
        init_process = process_lookup((uint32_t)pcb->init_pid);
    }
    
    if (init_process == NULL) {
        interrupts_restore(flags);
        return;
    }

    process_t *child;
    while ((child = (process_t *)queue_pop(process->children)) != NULL) {
        child->ppid = init_process->pid;
        add_child(init_process, child);
    }
    queue_destroy(process->children, NULL);
    process->children = NULL;
    interrupts_restore(flags);
}

int32_t process_wait_pid(uint32_t pid) { 

    process_t * current = scheduler_current();

    if (current == NULL) {
        return -1;
    }

    process_t *child = process_lookup(pid);

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
    process_t *current = scheduler_current();

    if (current == NULL) {
        return -1;
    }

    if (current->children == NULL || queue_is_empty(current->children)) {
        return 0;
    }

    size_t children = queue_size(current->children);
    int32_t reaped = 0;

    for (size_t i = 0; i < children; i++) {
        process_t *child = (process_t *)queue_pop(current->children);
        if (child == NULL) {
            continue;
        }

        if (!is_child(current, child)) {
            queue_push(current->children, child);
            continue;
        }

        if (child->state != PROCESS_STATE_TERMINATED) {
            queue_push(current->children, child);
            continue;
        }

        if (reap_child_process(child) == 0) {
            reaped++;
        }
    }

    return reaped;
}

void add_child(process_t *parent, process_t *child) {
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

int give_foreground_to(uint32_t target_pid) {
    if (pcb == NULL) {
        return -1;
    }

    uint64_t flags = interrupts_save_and_disable();
    int32_t current_pid = pcb->running_pid;
    interrupts_restore(flags);

    if (current_pid == -1) {
        return -1;
    }

    if (pcb->foreground_pid != current_pid) {
        return -1;
    }

    process_t *target = process_lookup(target_pid);
    if (target == NULL) {
        return -1;
    }

    pcb->foreground_pid = (int32_t)target_pid;
    return 0;
}

int get_foreground_process_pid(void) { 
    
    if (pcb == NULL) {
        return -1;
    }
    
    return pcb->foreground_pid; 
}
