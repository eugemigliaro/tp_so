#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <test.h>
#include <sys.h>

enum process_test_state {
    PROCESS_TEST_RUNNING,
    PROCESS_TEST_BLOCKED,
    PROCESS_TEST_KILLED
};

typedef struct {
    int32_t pid;
    enum process_test_state state;
} process_request_t;

#define PROCESS_TEST_DEFAULT_PRIORITY 2
#define PROCESS_TEST_FOREGROUND 1

static uint32_t random_seed_z = 362436069;
static uint32_t random_seed_w = 521288629;

static uint32_t rand_uint(void) {
    random_seed_z = 36969 * (random_seed_z & 65535) + (random_seed_z >> 16);
    random_seed_w = 18000 * (random_seed_w & 65535) + (random_seed_w >> 16);
    return (random_seed_z << 16) + random_seed_w;
}

static uint32_t rand_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    uint32_t u = rand_uint();
    return (uint32_t)((u + 1.0) * 2.328306435454494e-10 * max);
}

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

static void endless_loop_process(int argc, char **argv) {
    (void)argc;
    (void)argv;

    while (1) {
        processYield();
    }
}

static int32_t create_endless_loop(void) {
    static char *const argv_endless[] = { "endless_loop", NULL };
    return processCreate(endless_loop_process, 1, (char **)argv_endless, PROCESS_TEST_DEFAULT_PRIORITY, PROCESS_TEST_FOREGROUND);
}

static int32_t kill_process(int32_t pid) {
    return processKill((uint64_t)pid);
}

static int32_t block_process(int32_t pid) {
    return processBlock((uint64_t)pid);
}

static int32_t unblock_process(int32_t pid) {
    return processUnblock((uint64_t)pid);
}

uint64_t test_processes(uint64_t argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: test_processes <max_processes>\n");
        return (uint64_t)-1;
    }

    int64_t parsed = parse_positive(argv[0]);
    if (parsed <= 0) {
        printf("test_processes: invalid max_processes parameter\n");
        return (uint64_t)-1;
    }

    uint64_t max_processes = (uint64_t)parsed;

    process_request_t *process_requests = malloc(sizeof(process_request_t) * max_processes);
    if (process_requests == NULL) {
        printf("test_processes: ERROR allocating tracking array\n");
        return (uint64_t)-1;
    }

    uint64_t it = 0;

    while (1) {
        uint64_t alive = 0;

        for (uint64_t i = 0; i < max_processes; i++) {
            int32_t pid = create_endless_loop();
            if (pid < 0) {
                printf("test_processes: ERROR creating process\n");
                free(process_requests);
                return (uint64_t)-1;
            }
            process_requests[i].pid = pid;
            process_requests[i].state = PROCESS_TEST_RUNNING;
            alive++;
        }

        while (alive > 0) {
            for (uint64_t i = 0; i < max_processes; i++) {
                uint8_t action = (uint8_t)(rand_uniform(100) & 0x01);

                switch (action) {
                    case 0:
                        if (process_requests[i].state == PROCESS_TEST_RUNNING || process_requests[i].state == PROCESS_TEST_BLOCKED) {
                            if (kill_process(process_requests[i].pid) == -1) {
                                printf("test_processes: ERROR killing process\n");
                                free(process_requests);
                                return (uint64_t)-1;
                            }
                            // Wait for the killed process to be reaped
                            if (processWaitPid((uint64_t)process_requests[i].pid) == -1) {
                                printf("test_processes: ERROR waiting for killed process\n");
                                free(process_requests);
                                return (uint64_t)-1;
                            }
                            process_requests[i].state = PROCESS_TEST_KILLED;
                            alive--;
                        }
                        break;

                    case 1:
                        if (process_requests[i].state == PROCESS_TEST_RUNNING) {
                            if (block_process(process_requests[i].pid) == -1) {
                                printf("test_processes: ERROR blocking process\n");
                                free(process_requests);
                                return (uint64_t)-1;
                            }
                            process_requests[i].state = PROCESS_TEST_BLOCKED;
                        }
                        break;

                    default:
                        break;
                }
            }

            for (uint64_t i = 0; i < max_processes; i++) {
                if (process_requests[i].state == PROCESS_TEST_BLOCKED && (rand_uniform(100) & 0x01)) {
                    if (unblock_process(process_requests[i].pid) == -1) {
                        printf("test_processes: ERROR unblocking process\n");
                        free(process_requests);
                        return (uint64_t)-1;
                    }
                    process_requests[i].state = PROCESS_TEST_RUNNING;
                }
            }

            processYield();
        }

        printf("Iteracion : %d\n", it++);

        processYield();
    }
}
