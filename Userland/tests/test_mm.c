#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <test.h>
#include "test_util.h"

#define MAX_BLOCKS 128

typedef struct {
    void *address;
    uint32_t size;
} mm_rq;

uint64_t test_mm(uint64_t argc, char *argv[]) {
    mm_rq mm_rqs[MAX_BLOCKS];
    uint8_t rq;
    uint32_t total;
    uint64_t max_memory;

    if (argc != 1) {
        return (uint64_t)-1;
    }

    int64_t parsed = satoi(argv[0]);
    if (parsed <= 0) {
        return (uint64_t)-1;
    }
    max_memory = (uint64_t)parsed;

    uint64_t iteration = 0;

    while (1) {
        rq = 0;
        total = 0;

        while (rq < MAX_BLOCKS && total < max_memory) {
            uint64_t remaining64 = max_memory - total;
            if (remaining64 <= 1) {
                break;
            }
            uint32_t remaining = (uint32_t)remaining64;

            mm_rqs[rq].size = GetUniform(remaining - 1) + 1;
            mm_rqs[rq].address = malloc(mm_rqs[rq].size);

            if (mm_rqs[rq].address != NULL) {
                total += mm_rqs[rq].size;
                rq++;
            }
        }

        for (uint32_t i = 0; i < rq; i++) {
            if (mm_rqs[i].address != NULL) {
                memset(mm_rqs[i].address, (int)i, mm_rqs[i].size);
            }
        }

        for (uint32_t i = 0; i < rq; i++) {
            if (mm_rqs[i].address != NULL) {
                if (!memcheck(mm_rqs[i].address, (uint8_t)i, mm_rqs[i].size)) {
                    printf("test_mm ERROR\n");
                    return (uint64_t)-1;
                }
            }
        }

        for (uint32_t i = 0; i < rq; i++) {
            if (mm_rqs[i].address != NULL) {
                free(mm_rqs[i].address);
            }
        }

        printf("test_mm iteration %d\n", iteration++);
    }
}
