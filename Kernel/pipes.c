#include <pipes.h>
#include <sem.h>
#include <string.h>
#include <memoryManager.h>
#include <process.h>
#include <lib.h>
#include <stdbool.h>

#define SEM_NAME_LENGTH 9
#define NEXT_PIPE_ID(pipe) ((pipe)->id + 1) % MAX_PIPES
#define PREV_PIPE_ID(pipe) ((pipe)->id - 1 + MAX_PIPES) % MAX_PIPES
#define NEXT_BUFFER_IDX(idx) (((idx) + 1) % PIPE_BUFFER_SIZE)

struct pipe {
	uint8_t id;
	uint8_t buffer[PIPE_BUFFER_SIZE];
	uint64_t read_idx;
	uint64_t write_idx;
	uint64_t data_count;
	sem_t *can_read;
	sem_t *can_write;
	sem_t *mutex;
	uint8_t attached_count;
	uint32_t waiting_readers;
	uint32_t waiting_writers;
};

static pipe_t pipes[MAX_PIPES];
static uint8_t next_pipe_id = 0;
static char sem_name[SEM_NAME_LENGTH];

static void destroy_pipe(uint8_t id, pipe_t pipe);

void init_pipes(void) {
  strcpy(sem_name, "pipe_XXX");
  sem_name[SEM_NAME_LENGTH - 1] = '\0';

  return;
}

static void set_sem_name_number(void) {
	sem_name[5] = '0' + (next_pipe_id / 10);
	sem_name[6] = '0' + (next_pipe_id % 10);
}

static pipe_t create_pipe(void) {
	pipe_t new_pipe = mem_alloc(sizeof(struct pipe));
	if (new_pipe == NULL) {
		return NULL;
	}

	new_pipe->id = next_pipe_id;
	new_pipe->read_idx = 0;
	new_pipe->write_idx = 0;
	new_pipe->data_count = 0;
	new_pipe->attached_count = 0;
	new_pipe->waiting_readers = 0;
	new_pipe->waiting_writers = 0;

	set_sem_name_number();

	sem_name[7] = 'R';
	new_pipe->can_read = sem_create();
	if (new_pipe->can_read == NULL) {
		mem_free(new_pipe);
		return NULL;
	}
	sem_init(new_pipe->can_read, sem_name, 0);

	sem_name[7] = 'W';
	new_pipe->can_write = sem_create();
	if (new_pipe->can_write == NULL) {
		sem_destroy(new_pipe->can_read);
		mem_free(new_pipe);
		return NULL;
	}
	sem_init(new_pipe->can_write, sem_name, 0);

	sem_name[7] = 'M';
	new_pipe->mutex = sem_create();
	if (new_pipe->mutex == NULL) {
		sem_destroy(new_pipe->can_read);
		sem_destroy(new_pipe->can_write);
		mem_free(new_pipe);
		return NULL;
	}
	sem_init(new_pipe->mutex, sem_name, 1);

	return new_pipe;
}

int open_pipe(void) {
  if (next_pipe_id >= MAX_PIPES) {
    return -1;
  }

  pipe_t new_pipe = create_pipe();
  if (new_pipe == NULL) {
    return -1;
  }

  pipes[next_pipe_id] = new_pipe;
  return next_pipe_id++;
}

int attach_to_pipe(uint8_t id) {
  if (id >= MAX_PIPES || pipes[id] == NULL) {
    return -1;
  }

  pipe_t pipe = pipes[id];

  sem_wait(pipe->mutex);
  pipe->attached_count++;
  sem_post(pipe->mutex);

  return 0;
}

int read_pipe(uint8_t id, uint8_t * buffer, uint64_t bytes) {
	if (buffer == NULL) {
		return -1;
	}

	if (bytes == 0) {
		return 0;
	}

	if (id >= MAX_PIPES || pipes[id] == NULL) {
		return -1;
	}

	pipe_t pipe = pipes[id];
	uint64_t read_bytes = 0;

	while (read_bytes < bytes) {
		sem_wait(pipe->mutex);
		if (pipe->data_count == 0) {
			if (pipe->attached_count <= 1) {
				sem_post(pipe->mutex);
				break;
			}

			pipe->waiting_readers++;
			sem_post(pipe->mutex);

			sem_wait(pipe->can_read);
			sem_wait(pipe->mutex);
			pipe->waiting_readers--;

			if (pipe->data_count == 0) {
				sem_post(pipe->mutex);
				continue;
			}
		}

		buffer[read_bytes] = pipe->buffer[pipe->read_idx];
		pipe->read_idx = NEXT_BUFFER_IDX(pipe->read_idx);
		pipe->data_count--;

		sem_post(pipe->mutex);
		sem_post(pipe->can_write);

		read_bytes++;
	}

	return (int)read_bytes;
}

int write_pipe(uint8_t id, const uint8_t * buffer, uint64_t bytes) {
	if (buffer == NULL) {
		return -1;
	}

	if (bytes == 0) {
		return 0;
	}

	if (id >= MAX_PIPES || pipes[id] == NULL) {
		return -1;
	}

	pipe_t pipe = pipes[id];
	uint64_t written_bytes = 0;
	bool signaled_reader = false;


	while (written_bytes < bytes) {
		sem_wait(pipe->mutex);
		if (pipe->data_count == PIPE_BUFFER_SIZE) {
			pipe->waiting_writers++;
			sem_post(pipe->mutex);

			sem_wait(pipe->can_write);
			sem_wait(pipe->mutex);
			pipe->waiting_writers--;

			if (pipe->data_count == PIPE_BUFFER_SIZE) {
				sem_post(pipe->mutex);
				continue;
			}
		}

		pipe->buffer[pipe->write_idx] = buffer[written_bytes];
		pipe->write_idx = NEXT_BUFFER_IDX(pipe->write_idx);
		pipe->data_count++;
		if (pipe->waiting_readers > 0) {
			signaled_reader = true;
		}

		sem_post(pipe->mutex);
		sem_post(pipe->can_read);

		written_bytes++;
		if (signaled_reader) {
			_force_scheduler_interrupt();
			signaled_reader = false;
		}
	}

	return (int)written_bytes;
}

static void destroy_pipe(uint8_t id, pipe_t pipe) {
	if (pipe == NULL) {
		return;
	}

	sem_destroy(pipe->can_read);
	sem_destroy(pipe->can_write);
	sem_destroy(pipe->mutex);

	mem_free(pipe->can_read);
	mem_free(pipe->can_write);
	mem_free(pipe->mutex);
	mem_free(pipe);

	pipes[id] = NULL;
}

void unattach_from_pipe(uint8_t id, int pid) {
	if (id >= MAX_PIPES || pipes[id] == NULL || pid < PROCESS_FIRST_PID || pid >= PROCESS_FIRST_PID + PROCESS_MAX_PROCESSES) {
		return;
	}

	pipe_t pipe = pipes[id];

	sem_remove_process(pipe->can_read, pid);
	sem_remove_process(pipe->can_write, pid);

	sem_wait(pipe->mutex);
	if (pipe->attached_count > 0) {
		pipe->attached_count--;
	}
	sem_post(pipe->mutex);
}

int close_pipe(uint8_t id) {
	if (id >= MAX_PIPES || pipes[id] == NULL) {
		return -1;
	}

	pipe_t pipe = pipes[id];
	int is_std = (id == STDIN || id == STDOUT || id == STDERR);

	if (sem_wait(pipe->mutex) == -1) {
		return -1;
	}

	uint8_t attached = pipe->attached_count;
	int release_waiters = (!is_std && attached <= 1);
	int should_delete = (attached == 0) || (is_std && attached == 1);
	int readers_to_wake = release_waiters ? sem_waiting_count(pipe->can_read) : 0;
	int writers_to_wake = release_waiters ? sem_waiting_count(pipe->can_write) : 0;

	sem_post(pipe->mutex);

	while (readers_to_wake-- > 0) {
		sem_post(pipe->can_read);
	}

	while (writers_to_wake-- > 0) {
		sem_post(pipe->can_write);
	}

	if (should_delete) {
		destroy_pipe(id, pipe);
	}

	return 0;
}

void reset_pipes(void) {
	for (uint8_t i = 0; i < MAX_PIPES; i++) {
		if (pipes[i] != NULL) {
			destroy_pipe(i, pipes[i]);
		}
	}
	next_pipe_id = 0;
}

void set_next_id(uint8_t id) {
	next_pipe_id = id;
}
