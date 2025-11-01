#ifndef KERNEL_QUEUE_H
#define KERNEL_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct queue queue_t;
typedef struct queue_node queue_node_t;

typedef struct queue_iterator {
    const queue_node_t *current;
} queue_iterator_t;

queue_t *queue_create(void);
void queue_destroy(queue_t *queue, void (*destroy)(void *element));

void queue_clear(queue_t *queue, void (*destroy)(void *element));

bool queue_is_empty(const queue_t *queue);
size_t queue_size(const queue_t *queue);

bool queue_push(queue_t *queue, void *element);
void *queue_pop(queue_t *queue);
bool queue_remove(queue_t *queue, void *element);
void *queue_peek(const queue_t *queue);

queue_iterator_t queue_iter(const queue_t *queue);
bool queue_iter_has_next(const queue_iterator_t *iterator);
void *queue_iter_next(queue_iterator_t *iterator);

#endif
