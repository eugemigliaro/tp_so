#ifndef KERNEL_SEM_H
#define KERNEL_SEM_H

#include <stdint.h>
#include <queueADT.h>

typedef struct semaphore {
    char *name;
    uint32_t count;
    uint8_t lock;
    queue_t *waiting_processes;
} sem_t;

void sem_init(sem_t *sem, const char *name, uint32_t initial_count);
int sem_post(sem_t *sem);
int sem_wait(sem_t *sem);

/**
 * Finds a registered semaphore by name or NULL when not found.
 * Does not lock; the caller must synchronise if needed.
 */
sem_t *sem_find(const char *name);

#endif
