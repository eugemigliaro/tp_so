#include <sem.h>
#include <queueADT.h>
#include <stdint.h>
#include <lib.h>

typedef struct sem_queue{
    uint8_t lock;
    queue_t *semaphores;
}sem_queue_t;

typedef struct semaphore{
    char *name;
    uint32_t value;
    uint8_t lock;
    queue_t *waiting_processes;
}sem_t;

int sem_post(sem_t *sem){
    int ret = -1;
    if(sem == NULL){
        return ret;
    }

    semLock(&sem->lock);
    
    semUnlock(&sem->lock);
}