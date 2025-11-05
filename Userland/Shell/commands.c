#include "commands.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <test.h>

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
    // TODO: Implement - loop printing PID periodically
    // Based on TP2 reference: infinite loop with millisleep
    printf("loop: not yet implemented\n");
    return 0;
}

int kill(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return 1;
    }
    
    // TODO: Parse argv[1] as PID and call kill syscall
    printf("kill: not yet implemented\n");
    return 0;
}

int nice(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: nice <pid> <priority>\n");
        return 1;
    }
    
    // TODO: Parse argv[1] as PID, argv[2] as priority, call syscall
    printf("nice: not yet implemented\n");
    return 0;
}

int block(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: block <pid>\n");
        return 1;
    }
    
    // TODO: Parse argv[1] as PID, toggle block state
    printf("block: not yet implemented\n");
    return 0;
}

int cat(int argc, char *argv[]) {
    // TODO: Implement - read stdin, echo to stdout
    // Based on TP2 reference: simple loop reading and printing
    printf("cat: not yet implemented\n");
    return 0;
}

int wc(int argc, char *argv[]) {
    // TODO: Implement - count newlines from stdin
    printf("wc: not yet implemented\n");
    return 0;
}

int filter(int argc, char *argv[]) {
    // TODO: Implement - filter vowels from stdin
    printf("filter: not yet implemented\n");
    return 0;
}

int mvar(int argc, char *argv[]) {
    // TODO: Implement - multiple readers/writers with semaphores
    // This will spawn multiple processes
    printf("mvar: not yet implemented\n");
    return 0;
}
