#ifndef KERNEL_STRUCTURES_LIST_H
#define KERNEL_STRUCTURES_LIST_H

#include <stddef.h>
#include <stdint.h>

typedef struct list_node list_node_t;
typedef struct list list_t;

struct list_node {
    list_node_t *prev;
    list_node_t *next;
    list_t *owner;
};

struct list {
    list_node_t sentinel;
    size_t size;
};

void list_init(list_t *list);
void list_node_init(list_node_t *node);
int list_is_empty(const list_t *list);
size_t list_size(const list_t *list);
void list_push_front(list_t *list, list_node_t *node);
void list_push_back(list_t *list, list_node_t *node);
list_node_t *list_pop_front(list_t *list);
list_node_t *list_pop_back(list_t *list);
void list_remove(list_node_t *node);
list_node_t *list_peek_front(const list_t *list);
list_node_t *list_peek_back(const list_t *list);
int list_node_is_linked(const list_node_t *node);
list_t *list_node_owner(const list_node_t *node);

#define LIST_ENTRY(node_ptr, type, member) \
    ((type *)((char *)(node_ptr) - offsetof(type, member)))

#define LIST_FOR_EACH(pos, list_ptr) \
    for (list_node_t *(pos) = (list_ptr)->sentinel.next; \
         (pos) != &(list_ptr)->sentinel; \
         (pos) = (pos)->next)

#define LIST_FOR_EACH_SAFE(pos, tmp, list_ptr) \
    for (list_node_t *(pos) = (list_ptr)->sentinel.next, \
                     *(tmp) = (pos)->next; \
         (pos) != &(list_ptr)->sentinel; \
         (pos) = (tmp), (tmp) = (pos)->next)

#endif
