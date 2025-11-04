#ifndef KERNEL_PIPES_H
#define KERNEL_PIPES_H

#include <stdint.h>

#define MAX_PIPES 100
#define PIPE_BUFFER_SIZE (16 * 1024)

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef struct pipe * pipe_t;

void init_pipes(void);

int open_pipe(void);

int attach_to_pipe(uint8_t id);

int read_pipe(uint8_t id, uint8_t * buffer, uint64_t bytes);

int write_pipe(uint8_t id, const uint8_t * buffer, uint64_t bytes);

void unattach_from_pipe(uint8_t id, int pid);

int close_pipe(uint8_t id);

void free_pipes(void);

void reset_pipes(void);

void set_next_id(uint8_t id);

#endif
