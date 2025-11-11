// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stdio.h>

#include <test.h>
#include <sys.h>
#include "test_util.h"

#define TOTAL_PAIR_PROCESSES 2
#define PROCESS_PRIORITY_DEFAULT 2
#define PROCESS_FOREGROUND 1

static int64_t shared_value = 0;
static void *sync_sem = NULL;

static void slow_increment(int64_t *value, int64_t delta) {
    int64_t tmp = *value;
    processYield();
    tmp += delta;
    *value = tmp;
}

static void process_inc(int argc, char **argv) {
    if (argc < 3 || argv == NULL) {
        processExit(-1);
    }

    int64_t iterations = satoi(argv[0]);
    int64_t delta = satoi(argv[1]);
    int64_t use_sem = satoi(argv[2]);

    if (iterations <= 0 || delta == 0 || use_sem < 0) {
        processExit(-1);
    }

    if (use_sem && sync_sem == NULL) {
        processExit(-1);
    }

    for (int64_t i = 0; i < iterations; i++) {
        if (use_sem) {
            semWait(sync_sem);
        }
        slow_increment(&shared_value, delta);
        if (use_sem) {
            semPost(sync_sem);
        }
    }

    processExit(0);
}

static uint64_t run_sync_test(uint64_t iterations, uint8_t use_sem) {
    int32_t pids[2 * TOTAL_PAIR_PROCESSES];
    char iterations_str[24];
    char delta_pos[] = { '1', '\0' };
    char delta_neg[] = { '-', '1', '\0' };
    char use_sem_str[4];

    unsigned long long iter_copy = (unsigned long long)iterations;
    uint32_t idx = 0;
    if (iter_copy == 0) {
        iterations_str[idx++] = '0';
    } else {
        char tmp[24];
        uint32_t t = 0;
        while (iter_copy > 0 && t < sizeof(tmp)) {
            tmp[t++] = (char)('0' + (iter_copy % 10));
            iter_copy /= 10;
        }
        while (t > 0) {
            iterations_str[idx++] = tmp[--t];
        }
    }
    iterations_str[idx] = '\0';

    use_sem_str[0] = use_sem ? '1' : '0';
    use_sem_str[1] = '\0';

    shared_value = 0;

    for (int i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
        char *dec_argv[] = { iterations_str, delta_neg, use_sem_str, NULL };
        char *inc_argv[] = { iterations_str, delta_pos, use_sem_str, NULL };

        pids[i] = processCreate(process_inc, 3, dec_argv, PROCESS_PRIORITY_DEFAULT, PROCESS_FOREGROUND);
        if (pids[i] < 0) {
            printf("test_sync: failed to create decrement process\n");
            return (uint64_t)-1;
        }

        pids[i + TOTAL_PAIR_PROCESSES] = processCreate(process_inc, 3, inc_argv, PROCESS_PRIORITY_DEFAULT, PROCESS_FOREGROUND);
        if (pids[i + TOTAL_PAIR_PROCESSES] < 0) {
            printf("test_sync: failed to create increment process\n");
            return (uint64_t)-1;
        }
    }

    for (int i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
        processWaitPid((uint64_t)pids[i]);
        processWaitPid((uint64_t)pids[i + TOTAL_PAIR_PROCESSES]);
    }

    printf("Final value: %d\n", (int)shared_value);
    return 0;
}

uint64_t test_sync(uint64_t argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: test_sync <iterations>\n");
        return (uint64_t)-1;
    }

    int64_t iterations = satoi(argv[0]);
    if (iterations <= 0) {
        printf("test_sync: iterations must be positive\n");
        return (uint64_t)-1;
    }

    sync_sem = semOpen("test_sync_mutex", 1, 1);
    if (sync_sem == NULL) {
        printf("test_sync: failed to create semaphore\n");
        return (uint64_t)-1;
    }

    uint64_t result = run_sync_test((uint64_t)iterations, 1);

    semClose(sync_sem);
    sync_sem = NULL;
    return result;
}

uint64_t test_nosync(uint64_t argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: test_nosync <iterations>\n");
        return (uint64_t)-1;
    }

    int64_t iterations = satoi(argv[0]);
    if (iterations <= 0) {
        printf("test_nosync: iterations must be positive\n");
        return (uint64_t)-1;
    }

    return run_sync_test((uint64_t)iterations, 0);
}
