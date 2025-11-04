#include <fd.h>
#include <pipes.h>
#include <process.h>
#include <scheduler.h>
#include <fonts.h>
#include <interrupts.h>

static int is_valid_fd(int32_t fd) {
    return fd == READ_FD || fd == WRITE_FD || fd == ERROR_FD;
}

int setReadTarget(uint8_t fd_targets[3], uint8_t target) {
    if (fd_targets == NULL) {
        return -1;
    }

    fd_targets[READ_FD] = target;
    return 0;
}

int setWriteTarget(uint8_t fd_targets[3], uint8_t target) {
    if (fd_targets == NULL) {
        return -1;
    }

    fd_targets[WRITE_FD] = target;
    return 0;
}

int read(int32_t fd, uint8_t *user_buffer, int32_t count) {

    _cli();
    process_t *current = scheduler_current();
    _sti();

    if (current == NULL) {
        return -1;
    }

    if (user_buffer == NULL || count < 0 || !is_valid_fd(fd)) {
        return -1;
    }

    if (current->fd_targets[READ_FD] == STDIN && get_foreground_process_pid() != current->pid) {
        return -1; //lo deberia matar?

    }

    return read_pipe(current->fd_targets[fd], user_buffer, (uint64_t)count);
}

int write(int32_t fd, const uint8_t *user_buffer, int32_t count) {
    if (user_buffer == NULL || count < 0 || !is_valid_fd(fd)) {
        return -1;
    }

    if (fd == READ_FD) {
        return -1;
    }

    if (count == 0) {
        return 0;
    }

    process_t *current = scheduler_current();
    if (current == NULL) {
        return -1;
    }

    int pipe_id = current->fd_targets[fd];
    int written = write_pipe(pipe_id, user_buffer, (uint64_t)count);
    if (written < 0) {
        return written;
    }

    if ((fd == WRITE_FD && current->fd_targets[WRITE_FD] == STDOUT) ||
        (fd == ERROR_FD && current->fd_targets[ERROR_FD] == STDERR)) {
        for (int i = 0; i < written; i++) {
            uint8_t c;
            if (read_pipe(pipe_id, &c, 1) == -1) {
                return -1;
            }
            const char out_char = (char)c;
            printToFd(pipe_id, &out_char, 1);
        }
    }

    return written;
}
