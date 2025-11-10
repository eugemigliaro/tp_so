#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memoryManager.h>
#include "test_util.h"

#define MAX_BLOCKS 128

typedef struct MM_rq {
  void *address;
  uint32_t size;
} mm_rq;

uint64_t test_mm(uint64_t max_memory) {
  mm_rq mm_rqs[MAX_BLOCKS];
  uint8_t rq;
  uint32_t total;

  while (1) {
    rq = 0;
    total = 0;

    while (rq < MAX_BLOCKS && total < max_memory) {
      mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
      mm_rqs[rq].address = mem_alloc(mm_rqs[rq].size);

      if (mm_rqs[rq].address) {
        total += mm_rqs[rq].size;
        rq++;
      }
    }

    uint32_t i;
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        memset(mm_rqs[i].address, i, mm_rqs[i].size);

    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
          printf("test_mm ERROR\n");
          return -1;
        }

    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        mem_free(mm_rqs[i].address);
  }
}

int main(int argc, char *argv[]) {
  uint64_t max_memory = 50000;

  if (argc == 2) {
    max_memory = satoi(argv[1]);
  }

  mem_init();
  test_mm(max_memory);
  
  return 0;
}
