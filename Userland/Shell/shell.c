#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>

#include <sys.h>
#include <exceptions.h>

#include "commands.h"
#include "shell.h"

#ifdef ANSI_4_BIT_COLOR_SUPPORT
    #include <ansiColors.h>
#endif

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10
#define MAX_ARGS 64
#define MAX_ARGUMENT_COUNT MAX_ARGS
#define MAX_ARGUMENT_SIZE 256
#define MAX_PIPE_COMMANDS 2

#define CTRL_C 0x03
#define CTRL_D 0x04

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

typedef struct {
	char *name;
	int (*function)(int argc, char **argv);
	int argc;
	char *argv[MAX_ARGUMENT_COUNT];
	uint8_t isBuiltIn;
} ParsedCommand;

static int exit(int argc, char *argv[]);
static int history(int argc, char *argv[]);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static void appendCharacter(char c);
static void deleteCharacter(void);
static void emptyScreenBuffer(void);
static void printPrompt(void);
static void handleBackgroundChildren(void);
static int updateShellFdTargets(uint8_t read_target, uint8_t write_target, uint8_t error_target);
static int parseCommand(char *buffer, char *command, ParsedCommand *parsedCommand);
static void freeParsedCommands(ParsedCommand *commands, int count);
static void executeParsedCommands(ParsedCommand *commands, int count);
static void cleanupParsedCommand(ParsedCommand *command);
static char *trimWhitespace(char *str);

static uint8_t last_command_arrowed = 0;
static int prompt_dirty = 0;
static uint8_t shell_fd_targets[3] = {FD_STDIN, FD_STDOUT, FD_STDERR};

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
	 .func = divzero,
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
	 .func = invop,
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
	{.name = "time",
	 .func = time,
	 .description = "Prints the current time",
	 .isBuiltIn = 0},
	{.name = "tmm",
	 .func = testmm,
	 .description = "Memory manager stress test",
	 .isBuiltIn = 0},
	{.name = "tnosync",
	 .func = tnosync,
	 .description = "Shared counter race test (no semaphore)",
	 .isBuiltIn = 0},
	{.name = "tprio",
	 .func = testpriority,
	 .description = "Scheduler priority test",
	 .isBuiltIn = 0},
	{.name = "tproc",
	 .func = testprocesses,
	 .description = "Process lifecycle stress test",
	 .isBuiltIn = 0},
	{.name = "tsem",
	 .func = testsemaphore,
	 .description = "Semaphore sync test",
	 .isBuiltIn = 0},
	{.name = "tsync",
	 .func = testsync,
	 .description = "Shared counter sync test (semaphore protected)",
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

	if (buffer_dim == 0 && prompt_dirty) {
		putchar('\n');
		printPrompt();
	}

	if (buffer_dim >= MAX_BUFFER_SIZE - 1) {
		return;
	}

	buffer[buffer_dim] = c;
	command_history_buffer[buffer_dim] = c;
	buffer_dim++;
	putchar(c);
}


int main(void) {
  clearScreen();

  buffer[0] = 0;
  buffer_dim = 0;
  command_history_buffer[0] = 0;

	registerKey(KP_UP_KEY, printPreviousCommand);
  registerKey(KP_DOWN_KEY, printNextCommand);

	while (1) {
        handleBackgroundChildren();
        printPrompt();

        signed char c;

		while ((c = getchar()) != '\n') {
            handleBackgroundChildren();
			if (c == CTRL_C) {
				emptyScreenBuffer();
				break;
			}
			
			if (c == CTRL_D) {
				if (buffer_dim == 0) {
					printf("\n^D");
					return 0;
				}
				continue;
			}
			appendCharacter(c);
		}

		if (c != '\n') {
			buffer[0] = 0;
			command_history_buffer[0] = 0;
			buffer_dim = 0;
			continue;
		}

		putchar('\n');

		buffer[buffer_dim] = 0;
		command_history_buffer[buffer_dim] = 0;

        ParsedCommand parsedCommands[MAX_PIPE_COMMANDS];
        memset(parsedCommands, 0, sizeof(parsedCommands));
        char command_token[MAX_ARGUMENT_SIZE] = {0};
        int parsed = 0;
        int parse_error = 0;
        int printed_error_message = 0;

        char *first_command = buffer;
        char *pipe_separator = strchr(buffer, '|');
        char *second_command = NULL;

        if (pipe_separator != NULL) {
            *pipe_separator = '\0';
            second_command = pipe_separator + 1;
            char *extra_pipe = strchr(second_command, '|');
            if (extra_pipe != NULL) {
                printf("\e[0;33mOnly %d piped commands are supported\e[0m\n", MAX_PIPE_COMMANDS);
                parse_error = 1;
                printed_error_message = 1;
            }
        }

        if (!parse_error) {
            first_command = trimWhitespace(first_command);
            if (second_command != NULL) {
                second_command = trimWhitespace(second_command);
            }

            if (first_command[0] == '\0') {
                parse_error = 1;
            }
        }

        if (!parse_error) {
            command_token[0] = 0;
            int result = parseCommand(first_command, command_token, &parsedCommands[parsed]);
            if (result > 0) {
                parsed++;
            } else {
                parse_error = 1;
            }
        }

        if (!parse_error && second_command != NULL && second_command[0] != '\0') {
            command_token[0] = 0;
            int result = parseCommand(second_command, command_token, &parsedCommands[parsed]);
            if (result > 0) {
                parsed++;
            } else {
                parse_error = 1;
            }
        } else if (!parse_error && second_command != NULL && second_command[0] == '\0') {
            parse_error = 1;
        }

        if (parse_error || parsed == 0) {
            if (!printed_error_message && buffer_dim > 0 && command_token[0] != 0) {
                printf("\e[0;33mCommand not found:\e[0m %s\n", command_token);
            }
            freeParsedCommands(parsedCommands, parsed);
            buffer[0] = 0;
            command_history_buffer[0] = 0;
            buffer_dim = 0;
            continue;
        }

        strncpy(command_history[command_history_last], command_history_buffer, MAX_BUFFER_SIZE - 1);
        command_history[command_history_last][buffer_dim] = '\0';
        INC_MOD(command_history_last, HISTORY_SIZE);
        last_command_arrowed = command_history_last;

        executeParsedCommands(parsedCommands, parsed);
        freeParsedCommands(parsedCommands, parsed);

        buffer[0] = 0;
        command_history_buffer[0] = 0;
        buffer_dim = 0;
    }

    __builtin_unreachable();
    return 0;
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

static void printPrompt(void) {
	printf("\e[0mshell \e[0;32m$\e[0m ");
	prompt_dirty = 0;
}

static void handleBackgroundChildren(void) {
	int32_t reaped = processWaitChildren();
	if (reaped > 0) {
		prompt_dirty = 1;
	}
}

static void cleanupParsedCommand(ParsedCommand *command) {
	if (command == NULL) {
		return;
	}

	for (int i = 0; i < command->argc; i++) {
		if (command->argv[i] != NULL) {
			free(command->argv[i]);
			command->argv[i] = NULL;
		}
	}

	command->argc = 0;
	command->name = NULL;
	command->function = NULL;
	command->isBuiltIn = 0;
}

static void freeParsedCommands(ParsedCommand *commands, int count) {
	if (commands == NULL || count <= 0) {
		return;
	}

	for (int i = 0; i < count; i++) {
		cleanupParsedCommand(&commands[i]);
	}
}

static int parseCommand(char *buffer, char *command, ParsedCommand *parsedCommand) {
	if (command == NULL || parsedCommand == NULL) {
		return -1;
	}

	memset(parsedCommand, 0, sizeof(ParsedCommand));

	char *token = strtok(buffer, " ");
	while (token != NULL && strcmp(token, "|") == 0) {
		token = strtok(NULL, " ");
	}

	if (token == NULL) {
		return 0;
	}

	strncpy(command, token, MAX_ARGUMENT_SIZE - 1);
	command[MAX_ARGUMENT_SIZE - 1] = '\0';

	int command_index = -1;
	for (int i = 0; commands[i].name != NULL; i++) {
		if (strcmp(commands[i].name, command) == 0) {
			command_index = i;
			break;
		}
	}

	if (command_index == -1) {
		return -1;
	}

	parsedCommand->name = malloc(strlen(command) + 1);
	if (parsedCommand->name == NULL) {
		return -1;
	}
	strcpy(parsedCommand->name, command);
	parsedCommand->argv[0] = parsedCommand->name;
	parsedCommand->argc = 1;
	parsedCommand->function = commands[command_index].func;
	parsedCommand->isBuiltIn = commands[command_index].isBuiltIn;

	char *arg = strtok(NULL, " ");
	while (arg != NULL && strcmp(arg, "|") != 0 && parsedCommand->argc < MAX_ARGUMENT_COUNT - 1) {
		parsedCommand->argv[parsedCommand->argc] = malloc(strlen(arg) + 1);
		if (parsedCommand->argv[parsedCommand->argc] == NULL) {
			cleanupParsedCommand(parsedCommand);
			return -1;
		}
		strcpy(parsedCommand->argv[parsedCommand->argc], arg);
		parsedCommand->argc++;
		arg = strtok(NULL, " ");
	}

	parsedCommand->argv[parsedCommand->argc] = NULL;

	return 1;
}

static int updateShellFdTargets(uint8_t read_target, uint8_t write_target, uint8_t error_target) {
	if (setFdTargets(read_target, write_target, error_target) == 0) {
		shell_fd_targets[FD_STDIN] = read_target;
		shell_fd_targets[FD_STDOUT] = write_target;
		shell_fd_targets[FD_STDERR] = error_target;
		return 0;
	}
	return -1;
}

static void executeParsedCommands(ParsedCommand *commands, int count) {
	if (commands == NULL || count <= 0) {
		return;
	}

	if (count == 1 && commands[0].isBuiltIn) {
		last_command_output = commands[0].function(commands[0].argc, commands[0].argv);
		return;
	}

	int run_in_background = 0;
	if (commands[0].argc > 0) {
		char *last_arg = commands[0].argv[commands[0].argc - 1];
		if (last_arg != NULL && strcmp(last_arg, "&") == 0) {
			run_in_background = 1;
			free(commands[0].argv[commands[0].argc - 1]);
			commands[0].argv[commands[0].argc - 1] = NULL;
			commands[0].argc--;
		}
	}

	uint8_t original_in = shell_fd_targets[FD_STDIN];
	uint8_t original_out = shell_fd_targets[FD_STDOUT];
	uint8_t original_err = shell_fd_targets[FD_STDERR];

	if (count == 1) {
		int32_t pid = processCreate((void (*)(int, char **))commands[0].function, commands[0].argc, commands[0].argv, 1,
									run_in_background ? 0 : 1);
		if (pid < 0) {
			printf("\e[0;31mError creating process\e[0m\n");
			return;
		}

		if (run_in_background) {
			printf("[Background] PID: %d\n", pid);
			last_command_output = 0;
			prompt_dirty = 1;
			return;
		}

		last_command_output = processWaitPid(pid);
		return;
	}

	for (int i = 0; i < count; i++) {
		if (commands[i].isBuiltIn) {
			printf("\e[0;33mPipes are only supported for external commands\e[0m\n");
			return;
		}
	}

	int pipe_fd = openPipe();
	if (pipe_fd < 0) {
		printf("\e[0;31mError creating pipe\e[0m\n");
		return;
	}

	if (updateShellFdTargets(original_in, (uint8_t)pipe_fd, original_err) != 0) {
		printf("\e[0;31mFailed to configure pipe targets\e[0m\n");
		updateShellFdTargets(original_in, original_out, original_err);
		return;
	}

	int32_t left_pid =
		processCreate((void (*)(int, char **))commands[0].function, commands[0].argc, commands[0].argv, 1,
					  run_in_background ? 0 : 1);
	updateShellFdTargets(original_in, original_out, original_err);

	if (left_pid < 0) {
		printf("\e[0;31mError creating process\e[0m\n");
		return;
	}

	if (updateShellFdTargets((uint8_t)pipe_fd, original_out, original_err) != 0) {
		printf("\e[0;31mFailed to configure pipe targets\e[0m\n");
		updateShellFdTargets(original_in, original_out, original_err);
		return;
	}

	int32_t right_pid =
		processCreate((void (*)(int, char **))commands[1].function, commands[1].argc, commands[1].argv, 1, 0);
	updateShellFdTargets(original_in, original_out, original_err);

	if (right_pid < 0) {
		printf("\e[0;31mError creating process\e[0m\n");
		return;
	}

	if (run_in_background) {
		printf("[Background] PID: %d\n", left_pid);
		printf("[Background] PID: %d\n", right_pid);
		last_command_output = 0;
		prompt_dirty = 1;
		return;
	}

	processWaitPid(left_pid);
	last_command_output = processWaitPid(right_pid);
}

static char *trimWhitespace(char *str) {
	if (str == NULL) {
		return NULL;
	}

	while (*str != '\0' && isspace((unsigned char)*str)) {
		str++;
	}

	char *end = str + strlen(str);
	while (end > str) {
		end--;
		if (!isspace((unsigned char)*end)) {
			end++;
			break;
		}
	}

	*end = '\0';
	return str;
}

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
