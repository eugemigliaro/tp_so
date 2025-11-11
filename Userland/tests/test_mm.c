// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <test.h>
#include "test_util.h"

#define MM_TEST_MAX_BLOCKS 128U
#define MM_TEST_MIN_BLOCK_SIZE 1U

typedef struct {
  void *address;
  uint32_t size;
} mm_request_t;

uint64_t test_mm(uint64_t argc, char *argv[]) {

  mm_request_t requests[MM_TEST_MAX_BLOCKS];
  uint8_t rq;
  uint32_t total;
  uint32_t max_memory;

  if (argc != 1 || argv == NULL || argv[0] == NULL) {
    printf("Usage: tmm <max_memory_bytes>\n");
    return (uint64_t)-1;
  }

  int64_t parsed = satoi(argv[0]);
  if (parsed <= 0 || parsed > (int64_t)UINT32_MAX) {
    printf("tmm: max_memory_bytes must be between 1 and %u\n", (unsigned int)UINT32_MAX);
    return (uint64_t)-1;
  }

  max_memory = (uint32_t)parsed;

  while (1) {
    rq = 0;
    total = 0;

    // Request as many blocks as we can
    while (rq < MM_TEST_MAX_BLOCKS && total < max_memory) {
      uint32_t remaining = max_memory - total;
      if (remaining < MM_TEST_MIN_BLOCK_SIZE) {
        break;
      }

      uint32_t request_span = 0;
      if (remaining > MM_TEST_MIN_BLOCK_SIZE) {
        request_span = remaining - MM_TEST_MIN_BLOCK_SIZE;
      }

      requests[rq].size = GetUniform(request_span) + MM_TEST_MIN_BLOCK_SIZE;
      requests[rq].address = malloc(requests[rq].size);

      if (requests[rq].address) {
        total += requests[rq].size;
        rq++;
      }
    }

    // Set
    uint32_t i;
    for (i = 0; i < rq; i++) {
      if (requests[i].address) {
        memset(requests[i].address, (int)i, requests[i].size);
      }
    }

    // Check
    for (i = 0; i < rq; i++) {
      if (requests[i].address) {
        if (!memcheck(requests[i].address, (uint8_t)i, requests[i].size)) {
          printf("test_mm ERROR\n");
          return (uint64_t)-1;
        }
      }
    }

    // Free
    for (i = 0; i < rq; i++) {
      if (requests[i].address) {
        free(requests[i].address);
      }
    }
  }
}
