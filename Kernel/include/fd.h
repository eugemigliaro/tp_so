#ifndef KERNEL_FD_H
#define KERNEL_FD_H

#include <stdint.h>

#define READ_FD 0
#define WRITE_FD 1
#define ERROR_FD 2

int setReadTarget(uint8_t fd_targets[3], uint8_t target);
int setWriteTarget(uint8_t fd_targets[3], uint8_t target);
int setErrorTarget(uint8_t fd_targets[3], uint8_t target);
int set_fd_targets(uint8_t read_target, uint8_t write_target, uint8_t error_target);
int read(int32_t fd, uint8_t *user_buffer, int32_t count);
int write(int32_t fd, const uint8_t *user_buffer, int32_t count);

#endif
