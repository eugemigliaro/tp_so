#include <fd.h>
#include <pipes.h>
#include <process.h>
#include <scheduler.h>
#include <fonts.h>
#include <interrupts.h>

static int is_valid_fd(int32_t fd) {
    return fd == READ_FD || fd == WRITE_FD || fd == ERROR_FD;
}

int set_fd_targets(uint8_t read_target, uint8_t write_target, uint8_t error_target) {
    _cli();
    process_t *current = scheduler_current();
    _sti();

    if (current == NULL) {
        return -1;
    }

    uint8_t old_read = current->fd_targets[0];
    uint8_t old_write = current->fd_targets[1];
    uint8_t old_error = current->fd_targets[2];

    int attached_read = 0;
    int attached_write = 0;

    if (read_target != old_read) {
        if (attach_to_pipe(read_target) != 0) {
            return -1;
        }
        attached_read = 1;
    }
    if (write_target != old_write) {
        if (attach_to_pipe(write_target) != 0) {
            if (attached_read) {
                unattach_from_pipe(read_target, (int)current->pid);
            }
            return -1;
        }
        attached_write = 1;
    }
    if (error_target != old_error) {
        if (attach_to_pipe(error_target) != 0) {
            if (attached_read) {
                unattach_from_pipe(read_target, (int)current->pid);
            }
            if (attached_write) {
                unattach_from_pipe(write_target, (int)current->pid);
            }
            return -1;
        }
    }

    if (read_target != old_read) {
        setReadTarget(current->fd_targets, read_target);
        unattach_from_pipe(old_read, (int)current->pid);
    }
    if (write_target != old_write) {
        setWriteTarget(current->fd_targets, write_target);
        unattach_from_pipe(old_write, (int)current->pid);
    }
    if (error_target != old_error) {
        setErrorTarget(current->fd_targets, error_target);
        unattach_from_pipe(old_error, (int)current->pid);
    }

    return 0;
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

int setErrorTarget(uint8_t fd_targets[3], uint8_t target) {
    if (fd_targets == NULL) {
        return -1;
    }

    fd_targets[ERROR_FD] = target;
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

    _cli();
    process_t *current = scheduler_current();
    _sti();

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
