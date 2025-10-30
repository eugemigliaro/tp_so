#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <tests/test_mm.h>

#define MAX_BLOCKS 128
#define MAX_ITERATIONS 1024
#define DEFAULT_MAX_MEMORY_BYTES (32 * 1024)

typedef struct {
    void *address;
    uint32_t size;
} mm_request_t;

static uint32_t random_seed_z = 362436069;
static uint32_t random_seed_w = 521288629;

static uint32_t get_uint(void) {
    random_seed_z = 36969 * (random_seed_z & 65535) + (random_seed_z >> 16);
    random_seed_w = 18000 * (random_seed_w & 65535) + (random_seed_w >> 16);
    return (random_seed_z << 16) + random_seed_w;
}

static uint32_t get_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    uint32_t u = get_uint();
    return (uint32_t)((u + 1.0) * 2.328306435454494e-10 * max);
}

static void print_uint(uint32_t value) {
    char buffer[10];
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

static uint8_t memcheck(void *start, uint8_t value, uint32_t size) {
    uint8_t *p = (uint8_t *)start;
    for (uint32_t i = 0; i < size; i++, p++) {
        if (*p != value) {
            return 0;
        }
    }
    return 1;
}

static int64_t satoi(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    uint64_t i = 0;
    int64_t res = 0;
    int8_t sign = 1;

    if (str[i] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
        res = res * 10 + (str[i] - '0');
    }

    return res * sign;
}

uint64_t test_mm(uint64_t argc, char *argv[]) {
    mm_request_t requests[MAX_BLOCKS];
    uint64_t max_memory = DEFAULT_MAX_MEMORY_BYTES;

    if (argc > 1) {
        printf("test_mm expects at most one argument (max bytes to request).\n");
        return (uint64_t)-1;
    }

    if (argc == 1) {
        int64_t parsed = satoi(argv[0]);
        if (parsed <= 0) {
            printf("test_mm argument must be a positive integer (bytes to stress).\n");
            return (uint64_t)-1;
        }
        max_memory = (uint64_t)parsed;
    }

    if (max_memory > UINT32_MAX) {
        printf("Requested maximum truncated to ");
        print_uint((uint32_t)UINT32_MAX);
        printf(" bytes.\n");
        max_memory = UINT32_MAX;
    }

    uint8_t performed_allocations = 0;
    uint32_t peak_allocation = 0;

    for (uint32_t iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        uint8_t rq = 0;
        uint32_t total = 0;

        while (rq < MAX_BLOCKS && total < max_memory) {
            uint32_t remaining = (max_memory > total) ? (uint32_t)(max_memory - total) : 0;
            if (remaining == 0) {
                break;
            }

            uint32_t block_size = 1;
            if (remaining > 1) {
                block_size = get_uniform(remaining - 1) + 1;
            }

            requests[rq].size = block_size;
            requests[rq].address = malloc(requests[rq].size);

            if (requests[rq].address == NULL) {
                break;
            }

            performed_allocations = 1;
            total += requests[rq].size;
            if (total > peak_allocation) {
                peak_allocation = total;
            }
            rq++;
        }

        for (uint32_t i = 0; i < rq; i++) {
            if (requests[i].address != NULL) {
                uint8_t fill_value = (uint8_t)i;
                uint8_t *ptr = (uint8_t *)requests[i].address;
                for (uint32_t j = 0; j < requests[i].size; j++) {
                    ptr[j] = fill_value;
                }
            }
        }

        for (uint32_t i = 0; i < rq; i++) {
            if (requests[i].address != NULL) {
                if (!memcheck(requests[i].address, (uint8_t)i, requests[i].size)) {
                    printf("test_mm ERROR at iteration ");
                    print_uint(iteration);
                    printf(" block ");
                    print_uint(i);
                    printf(" (size ");
                    print_uint(requests[i].size);
                    printf(" bytes)\n");
                    for (uint32_t j = 0; j < rq; j++) {
                        if (requests[j].address != NULL) {
                            free(requests[j].address);
                        }
                    }
                    return (uint64_t)-1;
                }
            }
        }

        for (uint32_t i = 0; i < rq; i++) {
            if (requests[i].address != NULL) {
                free(requests[i].address);
            }
        }
    }

    if (!performed_allocations) {
        printf("test_mm could not allocate any memory. Ensure malloc/free are implemented.\n");
        return (uint64_t)-1;
    }

    printf("test_mm completed ");
    print_uint(MAX_ITERATIONS);
    printf(" iterations without errors (peak ");
    print_uint(peak_allocation);
    printf(" bytes).\n");
    return 0;
}
