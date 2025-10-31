#include <stdint.h>
#include <stdio.h>

#include <tests/test_priority.h>
#include <sys.h>

#define TOTAL_PROCESSES 3

#define PRIORITY_HIGHEST 0
#define PRIORITY_MEDIUM 2
#define PRIORITY_LOWEST 4
#define PRIORITY_DEFAULT PRIORITY_MEDIUM
#define PROCESS_FOREGROUND 1

static const int32_t priority_order[TOTAL_PROCESSES] = {
    PRIORITY_LOWEST,
    PRIORITY_MEDIUM,
    PRIORITY_HIGHEST
};

static uint64_t max_value = 0;

static int64_t parse_positive(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    int64_t result = 0;
    int8_t sign = 1;
    uint64_t i = 0;

    if (str[i] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
        result = result * 10 + (str[i] - '0');
    }

    return result * sign;
}

static int32_t get_pid(void) {
    return processGetPid();
}

static int32_t wait_process(int32_t pid) {
    return processWaitPid((uint64_t)pid);
}

static int32_t set_priority(int32_t pid, uint8_t priority) {
    return processSetPriority((uint64_t)pid, priority);
}

static int32_t block_process(int32_t pid) {
    return processBlock((uint64_t)pid);
}

static int32_t unblock_process(int32_t pid) {
    return processUnblock((uint64_t)pid);
}

static int32_t create_zero_to_max(void (*entry_point)(int, char **), const char *name) {
    char *argv_local[1];
    argv_local[0] = (char *)name;
    return processCreate(entry_point, 1, argv_local, PRIORITY_DEFAULT, PROCESS_FOREGROUND);
}

static void zero_to_max_process(int argc, char **argv) {
    (void)argc;
    (void)argv;

    uint64_t value = 0;
    int32_t pid = get_pid();

    while (value++ != max_value) {
        if ((value & 0x3FFFF) == 0) {
            processYield();
        }
    }

    printf("PROCESS %d DONE!\n", pid);
    processExit(0);
    while (1) {
        processYield();
    }
}

uint64_t test_prio(uint64_t argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: test_priority <max_value>\n");
        return (uint64_t)-1;
    }

    int64_t parsed = parse_positive(argv[0]);
    if (parsed <= 0) {
        printf("test_priority: invalid target value\n");
        return (uint64_t)-1;
    }

    max_value = (uint64_t)parsed;

    int32_t pids[TOTAL_PROCESSES] = {0};

    printf("SAME PRIORITY...\n");
    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        int32_t pid = create_zero_to_max(zero_to_max_process, "zero_to_max");
        if (pid < 0) {
            printf("test_priority: ERROR creating process\n");
            return (uint64_t)-1;
        }
        pids[i] = pid;
    }

    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        wait_process(pids[i]);
    }

    printf("SAME PRIORITY, THEN CHANGE IT...\n");
    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        int32_t pid = create_zero_to_max(zero_to_max_process, "zero_to_max");
        if (pid < 0) {
            printf("test_priority: ERROR creating process\n");
            return (uint64_t)-1;
        }
        pids[i] = pid;
        if (set_priority(pid, (uint8_t)priority_order[i]) == -1) {
            printf("test_priority: ERROR setting priority\n");
            return (uint64_t)-1;
        }
        printf("  PROCESS %d NEW PRIORITY: %d\n", pid, priority_order[i]);
    }

    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        wait_process(pids[i]);
    }

    printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");
    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        int32_t pid = create_zero_to_max(zero_to_max_process, "zero_to_max");
        if (pid < 0) {
            printf("test_priority: ERROR creating process\n");
            return (uint64_t)-1;
        }
        pids[i] = pid;
        if (block_process(pid) == -1) {
            printf("test_priority: ERROR blocking process\n");
            return (uint64_t)-1;
        }
        if (set_priority(pid, (uint8_t)priority_order[i]) == -1) {
            printf("test_priority: ERROR setting priority\n");
            return (uint64_t)-1;
        }
        printf("  PROCESS %d NEW PRIORITY: %d\n", pid, priority_order[i]);
    }

    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        if (unblock_process(pids[i]) == -1) {
            printf("test_priority: ERROR unblocking process\n");
            return (uint64_t)-1;
        }
    }

    for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
        wait_process(pids[i]);
    }

    return 0;
}
