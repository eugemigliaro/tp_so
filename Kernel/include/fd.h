#ifndef KERNEL_FD_H
#define KERNEL_FD_H

#include <stdint.h>

#define READ_FD 0
#define WRITE_FD 1

int setReadTarget(uint8_t fd_targets[2], uint8_t target);
int setWriteTarget(uint8_t fd_targets[2], uint8_t target);
int read(int32_t fd, uint8_t *user_buffer, int32_t count);
int write(int32_t fd, const uint8_t *user_buffer, int32_t count);

#endif
