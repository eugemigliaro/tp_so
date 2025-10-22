#include <collections/list.h>

static void list_attach(list_t *list, list_node_t *node, list_node_t *prev, list_node_t *next) {
    node->prev = prev;
    node->next = next;
    prev->next = node;
    next->prev = node;
    node->owner = list;
    list->size++;
}

void list_init(list_t *list) {
    if (list == NULL) {
        return;
    }

    list->sentinel.prev = &list->sentinel;
    list->sentinel.next = &list->sentinel;
    list->sentinel.owner = NULL;
    list->size = 0;
}

void list_node_init(list_node_t *node) {
    if (node == NULL) {
        return;
    }

    node->prev = NULL;
    node->next = NULL;
    node->owner = NULL;
}

int list_is_empty(const list_t *list) {
    return list == NULL || list->size == 0;
}

size_t list_size(const list_t *list) {
    return list == NULL ? 0 : list->size;
}

void list_push_front(list_t *list, list_node_t *node) {
    if (list == NULL || node == NULL) {
        return;
    }

    if (node->owner != NULL) {
        list_remove(node);
    }

    list_attach(list, node, &list->sentinel, list->sentinel.next);
}

void list_push_back(list_t *list, list_node_t *node) {
    if (list == NULL || node == NULL) {
        return;
    }

    if (node->owner != NULL) {
        list_remove(node);
    }

    list_attach(list, node, list->sentinel.prev, &list->sentinel);
}

static list_node_t *list_detach_front(list_t *list) {
    if (list_is_empty(list)) {
        return NULL;
    }

    return list->sentinel.next;
}

static list_node_t *list_detach_back(list_t *list) {
    if (list_is_empty(list)) {
        return NULL;
    }

    return list->sentinel.prev;
}

list_node_t *list_pop_front(list_t *list) {
    list_node_t *node = list_detach_front(list);
    if (node != NULL) {
        list_remove(node);
    }
    return node;
}

list_node_t *list_pop_back(list_t *list) {
    list_node_t *node = list_detach_back(list);
    if (node != NULL) {
        list_remove(node);
    }
    return node;
}

void list_remove(list_node_t *node) {
    if (node == NULL || node->owner == NULL) {
        return;
    }

    list_t *list = node->owner;

    node->prev->next = node->next;
    node->next->prev = node->prev;

    list->size--;
    node->prev = NULL;
    node->next = NULL;
    node->owner = NULL;
}

list_node_t *list_peek_front(const list_t *list) {
    if (list_is_empty(list)) {
        return NULL;
    }
    return list->sentinel.next;
}

list_node_t *list_peek_back(const list_t *list) {
    if (list_is_empty(list)) {
        return NULL;
    }
    return list->sentinel.prev;
}

int list_node_is_linked(const list_node_t *node) {
    return node != NULL && node->owner != NULL;
}

list_t *list_node_owner(const list_node_t *node) {
    return node == NULL ? NULL : node->owner;
}
