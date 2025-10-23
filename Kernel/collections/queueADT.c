#include "queueADT.h"
#include "memoryManager.h"

struct queue_node {
    void *data;
    struct queue_node *next;
};

struct queue {
    struct queue_node *head;
    struct queue_node *tail;
    size_t size;
};

queue_t *queue_create(void) {
    queue_t *queue = mem_alloc(sizeof(queue_t));
    if (queue == NULL) {
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

void queue_clear(queue_t *queue, void (*destroy)(void *element)) {
    if (queue == NULL) {
        return;
    }

    struct queue_node *current = queue->head;
    while (current != NULL) {
        struct queue_node *next = current->next;
        if (destroy != NULL) {
            destroy(current->data);
        }
        mem_free(current);
        current = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

void queue_destroy(queue_t *queue, void (*destroy)(void *element)) {
    if (queue == NULL) {
        return;
    }
    queue_clear(queue, destroy);
    mem_free(queue);
}

bool queue_is_empty(const queue_t *queue) {
    return queue == NULL || queue->size == 0;
}

size_t queue_size(const queue_t *queue) {
    return queue == NULL ? 0 : queue->size;
}

bool queue_push(queue_t *queue, void *element) {
    if (queue == NULL) {
        return false;
    }

    struct queue_node *node = mem_alloc(sizeof(struct queue_node));
    if (node == NULL) {
        return false;
    }

    node->data = element;
    node->next = NULL;

    if (queue->tail == NULL) {
        queue->head = node;
    } else {
        queue->tail->next = node;
    }
    queue->tail = node;
    queue->size++;
    return true;
}

void *queue_pop(queue_t *queue) {
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    struct queue_node *node = queue->head;
    void *data = node->data;

    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    mem_free(node);
    queue->size--;

    return data;
}

void *queue_peek(const queue_t *queue) {
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }
    return queue->head->data;
}

queue_iterator_t queue_iter(const queue_t *queue) {
    queue_iterator_t iterator = {0};
    if (queue != NULL) {
        iterator.current = queue->head;
    }
    return iterator;
}

bool queue_iter_has_next(const queue_iterator_t *iterator) {
    return iterator != NULL && iterator->current != NULL;
}

void *queue_iter_next(queue_iterator_t *iterator) {
    if (iterator == NULL || iterator->current == NULL) {
        return NULL;
    }

    const struct queue_node *node = iterator->current;
    iterator->current = node->next;
    return node->data;
}
