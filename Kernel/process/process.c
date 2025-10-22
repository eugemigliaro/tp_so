#include <process.h>
#include <lib.h>

static inline uint8_t normalize_priority(uint8_t priority) {
    if (priority < PROCESS_PRIORITY_MIN) {
        return PROCESS_PRIORITY_MIN;
    }
    if (priority > PROCESS_PRIORITY_MAX) {
        return PROCESS_PRIORITY_MAX;
    }
    return priority;
}

uint8_t process_priority_clamp(uint8_t priority) {
    return normalize_priority(priority);
}

int process_priority_valid(uint8_t priority) {
    return priority >= PROCESS_PRIORITY_MIN && priority <= PROCESS_PRIORITY_MAX;
}

void process_queue_init(process_queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    for (uint8_t i = 0; i < PROCESS_PRIORITY_LEVELS; i++) {
        list_init(&queue->levels[i]);
    }
    queue->process_count = 0;
}

static inline void queue_attach(process_queue_t *queue, pcb_t *pcb, void (*push_fn)(list_t *, list_node_t *)) {
    if (queue == NULL || pcb == NULL || push_fn == NULL) {
        return;
    }

    if (pcb->current_queue != NULL) {
        process_queue_remove(NULL, pcb);
    }

    pcb->priority = normalize_priority(pcb->priority);
    list_t *level = &queue->levels[pcb->priority];
    push_fn(level, &pcb->sched_node);
    pcb->current_queue = queue;
    queue->process_count++;
}

void process_queue_push(process_queue_t *queue, pcb_t *pcb) {
    queue_attach(queue, pcb, list_push_back);
}

void process_queue_push_front(process_queue_t *queue, pcb_t *pcb) {
    queue_attach(queue, pcb, list_push_front);
}

static pcb_t *queue_pop_from_level(process_queue_t *queue, uint8_t priority) {
    if (queue == NULL || !process_priority_valid(priority)) {
        return NULL;
    }

    list_t *level = &queue->levels[priority];
    list_node_t *node = list_pop_front(level);
    if (node == NULL) {
        return NULL;
    }

    pcb_t *pcb = LIST_ENTRY(node, pcb_t, sched_node);
    if (queue->process_count > 0) {
        queue->process_count--;
    }
    pcb->current_queue = NULL;
    return pcb;
}

pcb_t *process_queue_pop_priority(process_queue_t *queue, uint8_t priority) {
    return queue_pop_from_level(queue, priority);
}

pcb_t *process_queue_pop(process_queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }

    for (uint8_t priority = PROCESS_PRIORITY_MIN; priority <= PROCESS_PRIORITY_MAX; priority++) {
        pcb_t *pcb = queue_pop_from_level(queue, priority);
        if (pcb != NULL) {
            return pcb;
        }
    }
    return NULL;
}

static pcb_t *queue_peek_from_level(const process_queue_t *queue, uint8_t priority) {
    if (queue == NULL || !process_priority_valid(priority)) {
        return NULL;
    }

    list_node_t *node = list_peek_front(&queue->levels[priority]);
    if (node == NULL) {
        return NULL;
    }
    return LIST_ENTRY(node, pcb_t, sched_node);
}

pcb_t *process_queue_peek_priority(const process_queue_t *queue, uint8_t priority) {
    return queue_peek_from_level(queue, priority);
}

pcb_t *process_queue_peek(const process_queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }

    for (uint8_t priority = PROCESS_PRIORITY_MIN; priority <= PROCESS_PRIORITY_MAX; priority++) {
        pcb_t *pcb = queue_peek_from_level(queue, priority);
        if (pcb != NULL) {
            return pcb;
        }
    }
    return NULL;
}

void process_queue_remove(process_queue_t *queue, pcb_t *pcb) {
    if (pcb == NULL || pcb->current_queue == NULL) {
        return;
    }

    process_queue_t *owner = pcb->current_queue;
    if (queue != NULL && queue != owner) {
        return;
    }

    if (list_node_is_linked(&pcb->sched_node)) {
        list_remove(&pcb->sched_node);
        if (owner->process_count > 0) {
            owner->process_count--;
        }
    }
    pcb->current_queue = NULL;
}

int process_queue_is_empty(const process_queue_t *queue) {
    return queue == NULL || queue->process_count == 0;
}

size_t process_queue_size(const process_queue_t *queue) {
    return queue == NULL ? 0 : queue->process_count;
}

void pcb_init(pcb_t *pcb,
              uint64_t pid,
              const char *name,
              void *entry_point,
              void *stack_base,
              void *stack_top,
              uint8_t priority,
              process_foreground_t foreground) {
    if (pcb == NULL) {
        return;
    }

    memset(pcb, 0, sizeof(*pcb));
    list_init(&pcb->children);
    list_node_init(&pcb->sched_node);
    list_node_init(&pcb->sibling_node);

    pcb->pid = pid;
    pcb->ppid = 0;
    pcb->name = name;
    pcb->entry_point = entry_point;
    pcb->stack_base = stack_base;
    pcb->stack_top = stack_top;
    pcb->priority = normalize_priority(priority);
    pcb->foreground = foreground;
    pcb->state = PROCESS_STATE_NEW;
    pcb->ticks_remaining = 0;
    pcb->quantum_ticks = 0;
    pcb->exit_code = 0;
    pcb->parent = NULL;
    pcb->current_queue = NULL;

    pcb_reset_context(pcb);
}

void pcb_reset_context(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    memset(&pcb->context, 0, sizeof(cpu_context_t));
}

void pcb_set_priority(pcb_t *pcb, uint8_t priority) {
    if (pcb == NULL) {
        return;
    }

    pcb->priority = normalize_priority(priority);
}

void pcb_set_state(pcb_t *pcb, process_state_t state) {
    if (pcb == NULL) {
        return;
    }
    pcb->state = state;
}

void pcb_attach_child(pcb_t *parent, pcb_t *child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    pcb_detach_child(child);
    child->parent = parent;
    child->ppid = parent->pid;
    list_push_back(&parent->children, &child->sibling_node);
}

void pcb_detach_child(pcb_t *child) {
    if (child == NULL) {
        return;
    }

    if (list_node_is_linked(&child->sibling_node)) {
        list_remove(&child->sibling_node);
    }
    child->parent = NULL;
    child->ppid = 0;
}
