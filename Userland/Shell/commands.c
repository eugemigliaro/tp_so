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
    // TODO: Implement - count newlines from stdin
    printf("wc: not yet implemented\n");
    return 0;
}

int filter(int argc, char *argv[]) {
    // Si hay argumentos, procesar cada uno
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
        // Sin argumentos, leer de stdin (para pipes)
        int c;
        while ((c = getchar()) != -1) {
            if (!IS_VOCAL(c)) {
                putchar(c);
            }
        }
    }
    return 0;
}

int mvar(int argc, char *argv[]) {
    // TODO: Implement - multiple readers/writers with semaphores
    // This will spawn multiple processes
    printf("mvar: not yet implemented\n");
    return 0;
}
