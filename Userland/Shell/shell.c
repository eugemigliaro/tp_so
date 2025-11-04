#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <sys.h>
#include <exceptions.h>

#include <tests/test_mm.h>
#include <tests/test_processes.h>
#include <tests/test_priority.h>
#include <tests/test_semaphore.h>

#include "commands.h"
#include "shell_internal.h"

#ifdef ANSI_4_BIT_COLOR_SUPPORT
    #include <ansiColors.h>
#endif

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10
#define MAX_ARGS 64

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

// Built-in commands (POSIX built-ins or shell-specific)
static int exit(int argc, char *argv[]);
static int history(int argc, char *argv[]);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static void appendCharacter(char c);
static void deleteCharacter(void);
static void emptyScreenBuffer(void);

static uint8_t last_command_arrowed = 0;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
	{.name = "block",
	 .func = block,
	 .description = "Block/unblock a process by PID",
	 .isBuiltIn = 0},
	{.name = "cat",
	 .func = cat,
	 .description = "Print stdin as received",
	 .isBuiltIn = 0},
	{.name = "clear",
	 .func = clear,
	 .description = "Clears the screen",
	 .isBuiltIn = 0},
	{.name = "divzero",
	 .func = _divzero,
	 .description = "Generates a division by zero exception",
	 .isBuiltIn = 0},
	{.name = "exit",
	 .func = exit,
	 .description = "Command exits w/ the provided exit code or 0",
	 .isBuiltIn = 1},
	{.name = "filter",
	 .func = filter,
	 .description = "Filter vowels from input",
	 .isBuiltIn = 0},
	{.name = "font",
	 .func = font,
	 .description = "Increases or decreases the font size.\n\t\t\t\tUse:\n\t\t\t\t\t  + font increase\n\t\t\t\t\t  + font decrease",
	 .isBuiltIn = 0},
	{.name = "help",
	 .func = help,
	 .description = "Prints the available commands",
	 .isBuiltIn = 0},
	{.name = "history",
	 .func = history,
	 .description = "Prints the command history",
	 .isBuiltIn = 1},
	{.name = "invop",
	 .func = _invalidopcode,
	 .description = "Generates an invalid Opcode exception",
	 .isBuiltIn = 0},
	{.name = "kill",
	 .func = kill,
	 .description = "Kill a process by PID",
	 .isBuiltIn = 0},
	{.name = "loop",
	 .func = loop,
	 .description = "Loop printing PID periodically",
	 .isBuiltIn = 0},
	{.name = "man",
	 .func = man,
	 .description = "Prints the description of the provided command",
	 .isBuiltIn = 0},
	{.name = "mem",
	 .func = mem,
	 .description = "Print memory state",
	 .isBuiltIn = 0},
	{.name = "mvar",
	 .func = mvar,
	 .description = "Multiple readers/writers problem",
	 .isBuiltIn = 0},
	{.name = "nice",
	 .func = nice,
	 .description = "Change process priority",
	 .isBuiltIn = 0},
	{.name = "ps",
	 .func = ps,
	 .description = "List all processes with their properties",
	 .isBuiltIn = 0},
	{.name = "regs",
	 .func = regs,
	 .description = "Prints the register snapshot, if any",
	 .isBuiltIn = 0},
	{.name = "test_mm",
	 .func = testmm,
	 .description = "Stress tests dynamic memory (usage: test_mm [max_bytes])",
	 .isBuiltIn = 0},
	{.name = "test_priority",
	 .func = testpriority,
	 .description = "Exercises scheduler priorities (usage: test_priority <target_value>)",
	 .isBuiltIn = 0},
	{.name = "test_processes",
	 .func = testprocesses,
	 .description = "Stress tests process lifecycle (usage: test_processes <max_procs>)",
	 .isBuiltIn = 0},
	{.name = "test_semaphore",
	 .func = testsemaphore,
	 .description = "Runs two cooperating processes with a semaphore (usage: test_semaphore [iterations])",
	 .isBuiltIn = 0},
	{.name = "time",
	 .func = time,
	 .description = "Prints the current time",
	 .isBuiltIn = 0},
	{.name = "wc",
	 .func = wc,
	 .description = "Count lines from input",
	 .isBuiltIn = 0},
	{.name = NULL,
	 .func = NULL,
	 .description = NULL,
	 .isBuiltIn = 0}
};

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

int main() {
    clearScreen();

    registerKey(KP_UP_KEY, printPreviousCommand);
    registerKey(KP_DOWN_KEY, printNextCommand);

	while (1) {
        printf("\e[0mshell \e[0;32m$\e[0m ");

        signed char c;

		while ((c = getchar()) != '\n') {
			appendCharacter(c);
		}

		putchar('\n');

		buffer[buffer_dim] = 0;
		command_history_buffer[buffer_dim] = 0;

        // Parse buffer into argc/argv
        char *argv[MAX_ARGS];
        int argc = 0;
        
        char *token = strtok(buffer, " ");
        while (token != NULL && argc < MAX_ARGS - 1) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        if (argc == 0) {
            buffer[0] = 0;
            command_history_buffer[0] = 0;
            buffer_dim = 0;
            continue;
        }

        // Find and execute command
        int found = 0;
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(commands[i].name, argv[0]) == 0) {
                last_command_output = commands[i].func(argc, argv);
                strncpy(command_history[command_history_last], command_history_buffer, MAX_BUFFER_SIZE - 1);
                command_history[command_history_last][buffer_dim] = '\0';
                INC_MOD(command_history_last, HISTORY_SIZE);
                last_command_arrowed = command_history_last;
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("\e[0;33mCommand not found:\e[0m %s\n", argv[0]);
            // Save failed command in history too
            strncpy(command_history[command_history_last], command_history_buffer, MAX_BUFFER_SIZE - 1);
            command_history[command_history_last][buffer_dim] = '\0';
            INC_MOD(command_history_last, HISTORY_SIZE);
            last_command_arrowed = command_history_last;
        }
    
        buffer[0] = 0;
        command_history_buffer[0] = 0;
        buffer_dim = 0;
    }

    __builtin_unreachable();
    return 0;
}

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode) {
	last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
	if (command_history[last_command_arrowed][0] != 0) {
		emptyScreenBuffer();
		strcpy(buffer, command_history[last_command_arrowed]);
		strcpy(command_history_buffer, command_history[last_command_arrowed]);
		buffer_dim = strlen(command_history[last_command_arrowed]);
		printf("%s", command_history[last_command_arrowed]);
	}
}

static void printNextCommand(enum REGISTERABLE_KEYS scancode) {
	last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
	if (command_history[last_command_arrowed][0] != 0) {
		emptyScreenBuffer();
		strcpy(buffer, command_history[last_command_arrowed]);
		strcpy(command_history_buffer, command_history[last_command_arrowed]);
		buffer_dim = strlen(command_history[last_command_arrowed]);
		printf("%s", command_history[last_command_arrowed]);
	}
}

static void appendCharacter(char c) {
	if (c == '\b' || c == 127) {
		deleteCharacter();
		return;
	}

	if (buffer_dim >= MAX_BUFFER_SIZE - 1) {
		return;
	}

	buffer[buffer_dim] = c;
	command_history_buffer[buffer_dim] = c;
	buffer_dim++;
	putchar(c);
}

static void deleteCharacter(void) {
	if (buffer_dim == 0) {
		return;
	}

buffer_dim--;
buffer[buffer_dim] = 0;
command_history_buffer[buffer_dim] = 0;
clearScreenCharacter();
}

static void emptyScreenBuffer(void) {
	while (buffer_dim > 0) {
		deleteCharacter();
	}
}

// BUILT-IN COMMANDS IMPLEMENTATION

static int history(int argc, char *argv[]) {
    uint8_t last = command_history_last;
    DEC_MOD(last, HISTORY_SIZE);
    uint8_t i = 0;
    while (i < HISTORY_SIZE && command_history[last][0] != 0) {
        printf("%d. %s\n", i, command_history[last]);
        DEC_MOD(last, HISTORY_SIZE);
        i++;
    }
    return 0;
}

static int exit(int argc, char *argv[]) {
    if (argc >= 2) {
        int exit_code = 0;
        sscanf(argv[1], "%d", &exit_code);
        return exit_code;
    }
    return 0;
}
