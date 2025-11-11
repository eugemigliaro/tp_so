// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stddef.h>
#include <stdint.h>
#include <memoryManager.h>
#include <interrupts.h>
#include <lib.h>

#define HEAP_SIZE (1u << 19)

typedef struct Block {
    size_t size;
    int free;
    struct Block *next;
} Block;

#define BLOCK_SIZE sizeof(Block)

static uint8_t heap[HEAP_SIZE];
static Block *free_list = NULL;

void mem_init() {
    uint64_t flags = interrupts_save_and_disable();
    free_list = (Block *) heap;
    free_list->size = HEAP_SIZE - BLOCK_SIZE;
    free_list->free = 1;
    free_list->next = NULL;
    interrupts_restore(flags);
}

void *mem_alloc(size_t size) {
    uint64_t flags = interrupts_save_and_disable();
    Block *curr = free_list;

    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size > size + BLOCK_SIZE) {
                Block *new_block = (Block *)((uint8_t *)curr + BLOCK_SIZE + size);
                new_block->size = curr->size - size - BLOCK_SIZE;
                new_block->free = 1;
                new_block->next = curr->next;
                curr->next = new_block;
                curr->size = size;
            }
            curr->free = 0;
            void *result = (uint8_t *)curr + BLOCK_SIZE;
            interrupts_restore(flags);
            return result;
        }
        curr = curr->next;
    }

    interrupts_restore(flags);
    return NULL;
}


void mem_free(void *ptr) {
    uint64_t flags = interrupts_save_and_disable();
    if (!ptr) {
        interrupts_restore(flags);
        return;
    }

    Block *block = (Block *)((uint8_t *)ptr - BLOCK_SIZE);
    block->free = 1;
    interrupts_restore(flags);
}

void mem_status(size_t *total, size_t *used, size_t *available) {
    uint64_t flags = interrupts_save_and_disable();
    *total = HEAP_SIZE;
    *used = 0;
    *available = 0;

    Block *curr = free_list;
    while (curr) {
        if (curr->free) {
            *available += curr->size;
        } else {
            *used += curr->size;
        }
        curr = curr->next;
    }
    interrupts_restore(flags);
}

int32_t print_mem_status(void) {
    size_t total = 0, used = 0, available = 0;
    mem_status(&total, &used, &available);
    return print_mem_status_common(total, used, available);
}
