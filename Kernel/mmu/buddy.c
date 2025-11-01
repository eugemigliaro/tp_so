#include <stddef.h>
#include <stdint.h>
#include <memoryManager.h>
#include <interrupts.h>

#define HEAP_ORDER_MIN 5
#define HEAP_ORDER_MAX 19
#define HEAP_SIZE (1u << HEAP_ORDER_MAX)
#define TREE_LEVELS (HEAP_ORDER_MAX - HEAP_ORDER_MIN + 1)
#define NODE_COUNT ((1u << TREE_LEVELS) - 1)

typedef enum {
    NODE_FREE = 0,
    NODE_SPLIT,
    NODE_USED
} NodeState;

typedef struct BuddyNode {
    struct BuddyNode *parent;
    struct BuddyNode *left;
    struct BuddyNode *right;
    uint8_t order;
    NodeState state;
    uint8_t *base;
} BuddyNode;

typedef struct AllocationHeader {
    BuddyNode *node;
} AllocationHeader;

static uint8_t heap[HEAP_SIZE];
static BuddyNode nodes[NODE_COUNT];
static BuddyNode *root = NULL;

static size_t block_size(int order) {
    return (size_t)1u << order;
}

static BuddyNode *build_tree(int index, int order, uint8_t *base) {
    BuddyNode *node = &nodes[index];
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->order = (uint8_t)order;
    node->state = NODE_FREE;
    node->base = base;

    if (order > HEAP_ORDER_MIN) {
        int left_index = (index * 2) + 1;
        int right_index = left_index + 1;
        size_t half = block_size(order - 1);

        BuddyNode *left = build_tree(left_index, order - 1, base);
        BuddyNode *right = build_tree(right_index, order - 1, base + half);

        left->parent = node;
        right->parent = node;
        node->left = left;
        node->right = right;
    }

    return node;
}

static int required_order(size_t size) {
    int order = HEAP_ORDER_MIN;

    while (order <= HEAP_ORDER_MAX && block_size(order) < size) {
        order++;
    }

    return order;
}

static BuddyNode *acquire_node(BuddyNode *node, int order) {
    if (node == NULL) {
        return NULL;
    }

    if (node->state == NODE_USED) {
        return NULL;
    }

    if (node->order < order) {
        return NULL;
    }

    if (node->order == order) {
        if (node->state == NODE_FREE) {
            node->state = NODE_USED;
            return node;
        }
        return NULL;
    }

    if (node->left == NULL || node->right == NULL) {
        return NULL;
    }

    if (node->state == NODE_FREE) {
        node->state = NODE_SPLIT;
    }

    BuddyNode *result = acquire_node(node->left, order);
    if (result != NULL) {
        return result;
    }

    result = acquire_node(node->right, order);
    if (result != NULL) {
        return result;
    }

    if (node->left->state == NODE_FREE && node->right->state == NODE_FREE) {
        node->state = NODE_FREE;
    }

    return NULL;
}

static void coalesce_up(BuddyNode *node) {
    while (node != NULL) {
        if (node->left != NULL && node->right != NULL) {
            if (node->left->state == NODE_FREE && node->right->state == NODE_FREE) {
                node->state = NODE_FREE;
                node = node->parent;
            } else {
                node->state = NODE_SPLIT;
                node = NULL;
            }
        } else {
            node = node->parent;
        }
    }
}

static size_t free_bytes(const BuddyNode *node) {
    if (node == NULL) {
        return 0;
    }

    if (node->state == NODE_FREE) {
        return block_size(node->order);
    }

    if (node->state == NODE_SPLIT) {
        return free_bytes(node->left) + free_bytes(node->right);
    }

    return 0;
}

void mem_init(void) {
    _cli();
    root = build_tree(0, HEAP_ORDER_MAX, heap);
    _sti();
}

void *mem_alloc(size_t size) {
    _cli();
    if (root == NULL) {
        mem_init();
    }

    if (size == 0 || size > HEAP_SIZE) {
        _sti();
        return NULL;
    }

    if (size > HEAP_SIZE - sizeof(AllocationHeader)) {
        _sti();
        return NULL;
    }

    size_t total = size + sizeof(AllocationHeader);
    int order = required_order(total);

    if (order > HEAP_ORDER_MAX) {
        _sti();
        return NULL;
    }

    BuddyNode *node = acquire_node(root, order);
    if (node == NULL) {
        _sti();
        return NULL;
    }

    AllocationHeader *header = (AllocationHeader *)node->base;
    header->node = node;
    _sti();

    return node->base + sizeof(AllocationHeader);
}

void mem_free(void *ptr) {
    _cli();
    if (ptr == NULL) {
        _sti();
        return;
    }

    if (root == NULL) {
        _sti();
        return;
    }

    AllocationHeader *header = (AllocationHeader *)((uint8_t *)ptr - sizeof(AllocationHeader));
    BuddyNode *node = header->node;

    if (node == NULL || node->state != NODE_USED) {
        
        return;
    }

    header->node = NULL;
    node->state = NODE_FREE;
    coalesce_up(node->parent);
    _sti();
}

void mem_status(size_t *total, size_t *used, size_t *available) {
    _cli();
    if (root == NULL) {
        mem_init();
    }

    if (total != NULL) {
        *total = HEAP_SIZE;
    }

    size_t free_total = free_bytes(root);

    if (available != NULL) {
        *available = free_total;
    }

    if (used != NULL) {
        *used = HEAP_SIZE - free_total;
    }
    _sti();
}
