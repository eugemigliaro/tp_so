#include "commands.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <test.h>
#include <exceptions.h>

#define IS_VOCAL(c) ((c) == 'a' || (c) == 'e' || (c) == 'i' || (c) == 'o' || (c) == 'u' || \
                     (c) == 'A' || (c) == 'E' || (c) == 'I' || (c) == 'O' || (c) == 'U')

// ========== Exception command wrappers ==========

int divzero(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    _divzero();
    return 0;
}

int invop(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    _invalidopcode();
    return 0;
}

// ========== Existing commands (adapted to argc/argv pattern) ==========

int clear(int argc, char *argv[]) {
    clearScreen();
    return 0;
}

int help(int argc, char *argv[]) {
    printf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("%s%s\t ---\t%s\n", 
               commands[i].name, 
               strlen(commands[i].name) < 4 ? "\t" : "", 
               commands[i].description);
    }
    printf("\n");
    return 0;
}

int time(int argc, char *argv[]) {
	int hour, minute, second;
    getDate(&hour, &minute, &second);
    printf("Current time: %xh %xm %xs\n", hour, minute, second);
    return 0;
}

int font(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Usage: font [increase|decrease]\n");
        return 1;
    }

    if (strcasecmp(argv[1], "increase") == 0) {
        return increaseFontSize();
    } else if (strcasecmp(argv[1], "decrease") == 0) {
        return decreaseFontSize();
    }
    
    perror("Invalid argument. Use 'increase' or 'decrease'\n");
    return 1;
}

int man(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Usage: man <command>\n");
        return 1;
    }

    // Access commands array via extern (defined in shell.c)
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcasecmp(commands[i].name, argv[1]) == 0) {
            printf("Command: %s\nInformation: %s\n", commands[i].name, commands[i].description);
            return 0;
        }
    }

    perror("Command not found\n");
    return 1;
}

int regs(int argc, char *argv[]) {
    const static char * register_names[] = {
        "rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ", "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"
    };

    int64_t registers[18];

    uint8_t aux = getRegisterSnapshot(registers);
    
    if (aux == 0) {
        perror("No register snapshot available\n");
        return 1;
    }

    printf("Latest register snapshot:\n");

    for (int i = 0; i < 18; i++) {
        printf("\e[0;34m%s\e[0m: %x\n", register_names[i], registers[i]);
    }

    return 0;
}

// ========== TEST COMMANDS ==========
// These are thin wrappers that call the actual test functions

int testmm(int argc, char *argv[]) {
    return (int)test_mm(argc, argv);
}

int testprocesses(int argc, char *argv[]) {
    return (int)test_processes(argc, argv);
}

int testpriority(int argc, char *argv[]) {
    return (int)test_prio(argc, argv);
}

int testsemaphore(int argc, char *argv[]) {
    return (int)test_semaphore(argc, argv);
}

// ========== NEW COMMANDS for TP2 ==========

int mem(int argc, char *argv[]) {
    if (argc > 1) {
        printf("Usage: mem\n");
        return 1;
    }
    
    return printMemStatus();
}

int ps(int argc, char *argv[]) {
    if (argc > 1) {
        printf("Usage: ps\n");
        return 1;
    }
    
    return processList();
}

int loop(int argc, char *argv[]) {
    uint32_t seconds = 1; // Default: 1 second
    
    if (argc > 2) {
        printf("Usage: loop [seconds]\n");
        return 1;
    }
    
    if (argc == 2) {
        int parsed = atoi(argv[1]);
        if (parsed <= 0) {
            printf("Error: seconds must be a positive integer\n");
            return 1;
        }
        seconds = (uint32_t)parsed;
    }
    
    int32_t pid = processGetPid();
    
    while (1) {
        printf("Hello from PID: %d\n", pid);
        sleep(seconds * 1000); // Convert to milliseconds
    }
    
    return 0;
}

int kill(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return 1;
    }
    
    int pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: PID must be a positive integer\n");
        return 1;
    }
    
    int32_t result = processKill((uint64_t)pid);
    
    if (result < 0) {
        printf("Error: Could not kill process %d\n", pid);
        return 1;
    }
    
    printf("Process %d killed successfully\n", pid);
    return 0;
}

int nice(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: nice <pid> <priority>\n");
        return 1;
    }
    
    int pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: PID must be a positive integer\n");
        return 1;
    }
    
    int priority = atoi(argv[2]);
    if (priority < 0 || priority > 5) {
        printf("Error: Priority must be between 0 and 5\n");
        return 1;
    }
    
    int32_t result = processSetPriority((uint64_t)pid, (uint8_t)priority);
    
    if (result < 0) {
        printf("Error: Could not set priority for process %d\n", pid);
        return 1;
    }
    
    printf("Process %d priority set to %d successfully\n", pid, priority);
    return 0;
}

int block(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: block <pid>\n");
        return 1;
    }
    
    int pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: PID must be a positive integer\n");
        return 1;
    }
    
    int32_t result = processBlock((uint64_t)pid);
    
    if (result < 0) {
        result = processUnblock((uint64_t)pid);
        
        if (result < 0) {
            printf("Error: Could not toggle block state for process %d\n", pid);
            return 1;
        }
        
        printf("Process %d unblocked successfully\n", pid);
    } else {
        printf("Process %d blocked successfully\n", pid);
    }
    
    return 0;
}

int cat(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

int wc(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    int c, lines = 0;
    while (1) {
        c = getchar();
        if (c == -1) {
            break;
        }
        if (c == '\n') {
            lines++;
        }
    }
    printf("%d line%s in total\n", lines, lines == 1 ? "" : "s");
    return 0;
}

int filter(int argc, char *argv[]) {
    // Si hay argumentos
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            char *str = argv[i];
            for (int j = 0; str[j] != '\0'; j++) {
                if (!IS_VOCAL(str[j])) {
                    putchar(str[j]);
                }
            }
            if (i < argc - 1) {
                putchar(' ');
            }
        }
        putchar('\n');
    } else {
        // Sin argumentos, lee de stdin 
        int c;
        while ((c = getchar()) != -1) {
            if (!IS_VOCAL(c)) {
                putchar(c);
            }
        }
    }
    return 0;
}

char mvar_var[1] = {0};
void *mvar_empty_signal = NULL;
void *mvar_full_signal = NULL;

const char *const writer_ids[MVAR_MAX_WRITERS] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J"
};

const char *const reader_ids[MVAR_MAX_READERS] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"
};

static void mvar_writer(int argc, char **argv) {
    (void)argc;
    char *id = argv[1];

    for (int i = 0; i < 5; i++) {
        uint32_t sleep_ms = (rand() % 3000) + 1000; // Sleep between 1 and 4 seconds
        sleep(sleep_ms);
        semWait(mvar_empty_signal);
        mvar_var[0] = id[0];
        semPost(mvar_full_signal);
    }

    printf("[Writer %c] Finished\n", id[0]);

    return;
}

static void mvar_reader(int argc, char **argv) {
    (void)argc;
    char *id = argv[1];
    uint8_t not_end = 1;

    while (not_end) {
        uint32_t sleep_ms = (rand() % 3000) + 1000; // Sleep between 1 and 4 seconds
        sleep(sleep_ms);
        
        semWait(mvar_full_signal);
        char value = mvar_var[0];
        
        if (value == 0) {
            semPost(mvar_full_signal);
            not_end = 0;
        } else {
            char id_char = id[0];
            printf("[Reader %c] Read value: %c\n", id_char, value);
            semPost(mvar_empty_signal);
        }
    }

    printf("[Reader %c] Finished\n", id[0]);

    return;
}

int mvar(int argc, char *argv[]){
    if (argc < 3) {
        printf("Usage: mvar <escritores> <lectores>\n");
        return 1;
    }

    int writers = atoi(argv[1]);
    int readers = atoi(argv[2]);

    if (writers <= 0 || readers <= 0) {
        printf("Error: Both <escritores> and <lectores> must be positive integers\n");
        return 1;
    }

    mvar_empty_signal = semOpen("mvar_empty", 1, 1);
    mvar_full_signal = semOpen("mvar_full", 0, 1);

    if (writers > MVAR_MAX_WRITERS || readers > MVAR_MAX_READERS) {
        printf("Error: Too many readers or writers, maximum is 10 for both\n");
        return 1;
    }

    int32_t writer_pids[MVAR_MAX_WRITERS];
    int32_t reader_pids[MVAR_MAX_READERS];

    for (int i = 0; i < writers; i++) {
        char *writer_argv[] = { "mvar_writer", (char *)writer_ids[i], NULL };
        writer_pids[i] = processCreate((void (*)(int, char **))mvar_writer, 2, writer_argv, 1, 0);
        if (writer_pids[i] < 0) {
            printf("Error creating writer %d\n", i);
        }
    }

    for (int i = 0; i < readers; i++) {
        char *reader_argv[] = { "mvar_reader", (char *)reader_ids[i], NULL };
        reader_pids[i] = processCreate((void (*)(int, char **))mvar_reader, 2, reader_argv, 1, 0);
        if (reader_pids[i] < 0) {
            printf("Error creating reader %d\n", i);
        }
    }

    for (int i = 0; i < writers; i++) {
        if (writer_pids[i] >= 0) {
            processWaitPid((uint64_t)writer_pids[i]);
        }
    }

    for (int i = 0; i < readers; i++) {
        mvar_var[0] = 0;
        semPost(mvar_full_signal);
    }

    for (int i = 0; i < readers; i++) {
        if (reader_pids[i] >= 0) {
            processWaitPid((uint64_t)reader_pids[i]);
        }
    }

    semClose(mvar_empty_signal);
    semClose(mvar_full_signal);
    mvar_empty_signal = NULL;
    mvar_full_signal = NULL;

    return 0;
}
