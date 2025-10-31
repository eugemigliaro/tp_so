#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tests/test_semaphore.h>
#include <sys.h>

#define DEFAULT_ITERATIONS 3
#define PROCESS_PRIORITY_DEFAULT 2
#define PROCESS_FOREGROUND 1

static const char *SEM_NAME = "user_sem_turn";

static void *sync_sem = NULL;
static uint64_t total_iterations = DEFAULT_ITERATIONS;

static int64_t parse_positive(const char *str) {
    if (str == NULL || *str == '\0') {
        return -1;
    }

    int64_t result = 0;
    int64_t sign = 1;
    for (uint64_t i = 0; str[i] != '\0'; i++) {
        if (i == 0 && str[i] == '-') {
            sign = -1;
            continue;
        }
        if (str[i] < '0' || str[i] > '9') {
            return -1;
        }
        result = result * 10 + (str[i] - '0');
    }
    return result * sign;
}

static uint64_t parse_iterations(char *arg) {
    if (arg == NULL) {
        return DEFAULT_ITERATIONS;
    }
    int64_t parsed = parse_positive(arg);
    if (parsed <= 0) {
        return DEFAULT_ITERATIONS;
    }
    return (uint64_t)parsed;
}

static void print_cstr(const char *s) {
    if (s == NULL) {
        return;
    }
    while (*s != '\0') {
        putchar(*s++);
    }
}

static void print_uint(uint64_t value) {
    char buffer[20];
    uint32_t i = 0;

    if (value == 0) {
        putchar('0');
        return;
    }

    while (value > 0 && i < sizeof(buffer)) {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        putchar(buffer[--i]);
    }
}

static void print_line(const char *label, const char *msg, uint64_t iteration) {
    print_cstr(label);
    print_cstr(" ");
    print_cstr(msg);
    if (iteration > 0) {
        print_cstr(" (");
        print_uint(iteration);
        print_cstr(")");
    }
    print_cstr("\n");
}

static void process_hello_goodbye(const char *label) {
    print_line(label, "started", 0);
    for (uint64_t i = 0; i < total_iterations; i++) {
        if (semWait(sync_sem) != 0) {
            print_line(label, "failed sem_wait", 0);
            processExit(-1);
        }

        print_line(label, "hello", i + 1);

        sleep(1);

        print_line(label, "goodbye", i + 1);

        if (semPost(sync_sem) != 0) {
            print_line(label, "failed sem_post", 0);
            processExit(-1);
        }
    }

    print_line(label, "done", 0);
    processExit(0);
    __builtin_unreachable();
}

static void process_a(int argc, char **argv) {
    (void)argc;
    (void)argv;
    process_hello_goodbye("[Process A]");
}

static void process_b(int argc, char **argv) {
    (void)argc;
    (void)argv;
    process_hello_goodbye("    [Process B]");
}

uint64_t test_semaphore(uint64_t argc, char *argv[]) {
    total_iterations = DEFAULT_ITERATIONS;
    if (argc >= 1) {
        total_iterations = parse_iterations(argv[0]);
    }

    print_cstr("test_semaphore starting\n");

    sync_sem = semOpen(SEM_NAME, 1, 1);
    if (sync_sem == NULL) {
        print_cstr("test_semaphore: failed to open semaphore\n");
        return (uint64_t)-1;
    }

    char *argv_a[] = { "procA", NULL };
    char *argv_b[] = { "procB", NULL };

    int32_t pid_a = processCreate(process_a, 1, argv_a, PROCESS_PRIORITY_DEFAULT, PROCESS_FOREGROUND);
    if (pid_a < 0) {
        print_cstr("test_semaphore: failed to create process A\n");
        semClose(sync_sem);
        sync_sem = NULL;
        return (uint64_t)-1;
    }

    int32_t pid_b = processCreate(process_b, 1, argv_b, PROCESS_PRIORITY_DEFAULT, PROCESS_FOREGROUND);
    if (pid_b < 0) {
        print_cstr("test_semaphore: failed to create process B\n");
        processKill((uint64_t)pid_a);
        semClose(sync_sem);
        sync_sem = NULL;
        return (uint64_t)-1;
    }

    processWaitPid((uint64_t)pid_a);
    processWaitPid((uint64_t)pid_b);

    semClose(sync_sem);
    sync_sem = NULL;

    print_cstr("test_semaphore completed ");
    print_uint(total_iterations);
    print_cstr(" iterations per process.\n");
    return 0;
}
