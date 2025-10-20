#include <stddef.h>
#include <stdint.h>
#include <memoryManager.h>

#define HEAP_SIZE (4096 * 16)

typedef struct Block {
    size_t size;
    int free;
    struct Block *next;
} Block;

#define BLOCK_SIZE sizeof(Block)

static uint8_t heap[HEAP_SIZE];
static Block *free_list = NULL;

void mem_init() {
    free_list = (Block *) heap;
    free_list->size = HEAP_SIZE - BLOCK_SIZE;
    free_list->free = 1;
    free_list->next = NULL;
}

void *mem_alloc(size_t size) {
    Block *curr = free_list;

    while (curr) {
        if (curr->free && curr->size >= size) {
            // dividir bloque si sobra espacio
            if (curr->size > size + BLOCK_SIZE) {
                Block *new_block = (Block *)((uint8_t *)curr + BLOCK_SIZE + size);
                new_block->size = curr->size - size - BLOCK_SIZE;
                new_block->free = 1;
                new_block->next = curr->next;
                curr->next = new_block;
                curr->size = size;
            }
            curr->free = 0;
            return (uint8_t *)curr + BLOCK_SIZE;
        }
        curr = curr->next;
    }

    return NULL;
}


void mem_free(void *ptr) {
    if (!ptr) {
        return;
    }

    Block *block = (Block *)((uint8_t *)ptr - BLOCK_SIZE);
    block->free = 1;
}

void mem_status(size_t *total, size_t *used, size_t *available) {
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
}
