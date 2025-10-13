
#ifndef TP_SO_MEMORYMANAGER_H
#define TP_SO_MEMORYMANAGER_H

#include <stddef.h>

void mem_init(void);

void *mem_alloc(size_t size);

void mem_free(void *ptr);

void mem_status(size_t *total, size_t *used, size_t *available);

#endif //TP_SO_MEMORYMANAGER_H
