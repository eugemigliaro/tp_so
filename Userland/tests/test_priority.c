// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stdio.h>

#include <sys.h>
#include <test.h>
#include "test_util.h"

#define TOTAL_PROCESSES 3

#define LOWEST 5
#define MEDIUM 3
#define HIGHEST 0

#define PROCESS_FOREGROUND 1

static const uint8_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

static uint64_t max_value = 0;

static char zero_to_max_name[] = "zero_to_max";
static char *zero_to_max_argv[] = { zero_to_max_name, NULL };

static void zero_to_max(int argc, char **argv) {
    (void)argc;
    (void)argv;

    uint64_t value = 0;

    while (value++ != max_value)
        ;

    printf("PROCESS %d DONE!\n", processGetPid());
    processExit(0);
}

uint64_t test_prio(uint64_t argc, char *argv[]) {
    int32_t pids[TOTAL_PROCESSES];
    uint64_t i;

    if (argc != 1) {
        printf("test_prio expects a single argument (work units)\n");
        return (uint64_t)-1;
    }

    int64_t parsed = satoi(argv[0]);
    if (parsed <= 0) {
        printf("test_prio: argument must be a positive integer\n");
        return (uint64_t)-1;
    }
    max_value = (uint64_t)parsed;

    printf("SAME PRIORITY...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = processCreate(zero_to_max, 1, zero_to_max_argv, MEDIUM, PROCESS_FOREGROUND);
        if (pids[i] < 0) {
            printf("test_prio: failed to create process\n");
            return (uint64_t)-1;
        }
    }

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        processWaitPid((uint64_t)pids[i]);
    }

    printf("SAME PRIORITY, THEN CHANGE IT...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = processCreate(zero_to_max, 1, zero_to_max_argv, MEDIUM, PROCESS_FOREGROUND);
        if (pids[i] < 0) {
            printf("test_prio: failed to create process\n");
            return (uint64_t)-1;
        }
        processSetPriority((uint64_t)pids[i], prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        processWaitPid((uint64_t)pids[i]);
    }

    printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = processCreate(zero_to_max, 1, zero_to_max_argv, MEDIUM, PROCESS_FOREGROUND);
        if (pids[i] < 0) {
            printf("test_prio: failed to create process\n");
            return (uint64_t)-1;
        }
        processBlock((uint64_t)pids[i]);
        processSetPriority((uint64_t)pids[i], prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        processUnblock((uint64_t)pids[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        processWaitPid((uint64_t)pids[i]);
    }

    return 0;
}
