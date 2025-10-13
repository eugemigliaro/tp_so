#include "test_mm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BLOCKS 128

typedef struct {
    void *address;
    uint32_t size;
} MemoryRequest;

static uint32_t get_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    return (uint32_t)(rand() % (max + 1));
}

static int memcheck_block(void *address, uint8_t value, uint32_t size) {
    uint8_t *ptr = (uint8_t *)address;
    for (uint32_t i = 0; i < size; i++) {
        if (ptr[i] != value) {
            return 0;
        }
    }
    return 1;
}

int run_mm_test(uint32_t max_memory) {
    if (max_memory == 0) {
        printf("test_mm: max memory must be greater than zero\n");
        return -1;
    }

    MemoryRequest requests[MAX_BLOCKS];

    while (1) {
        memset(requests, 0, sizeof(requests));

        uint8_t request_count = 0;
        uint32_t total = 0;

        while (request_count < MAX_BLOCKS && total < max_memory) {
            uint32_t remaining = max_memory - total;
            if (remaining == 0) {
                break;
            }

            uint32_t next_size = 1;
            if (remaining > 1) {
                next_size += get_uniform(remaining - 1);
            }

            requests[request_count].size = next_size;
            requests[request_count].address = malloc(next_size);

            if (requests[request_count].address != NULL) {
                total += next_size;
                request_count++;
            } else {
                break;
            }
        }

        for (uint32_t i = 0; i < request_count; i++) {
            if (requests[i].address != NULL) {
                memset(requests[i].address, (int)i, requests[i].size);
            }
        }

        for (uint32_t i = 0; i < request_count; i++) {
            if (requests[i].address != NULL) {
                if (!memcheck_block(requests[i].address, (uint8_t)i, requests[i].size)) {
                    printf("test_mm: memory corruption detected on block %u\n", i);
                    return -1;
                }
            }
        }

        for (uint32_t i = 0; i < request_count; i++) {
            if (requests[i].address != NULL) {
                free(requests[i].address);
            }
        }
    }

    return 0;
}
