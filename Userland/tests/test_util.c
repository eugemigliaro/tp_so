#include <stdint.h>
#include <stdio.h>

#include <syscalls.h>
#include "test_util.h"

static uint32_t m_z = 362436069;
static uint32_t m_w = 521288629;

uint32_t GetUint(void) {
    m_z = 36969U * (m_z & 65535U) + (m_z >> 16);
    m_w = 18000U * (m_w & 65535U) + (m_w >> 16);
    return (m_z << 16) + m_w;
}

uint32_t GetUniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    uint32_t u = GetUint();
    return (uint32_t)((u + 1.0) * 2.328306435454494e-10 * max);
}

uint8_t memcheck(void *start, uint8_t value, uint32_t size) {
    uint8_t *p = (uint8_t *)start;
    for (uint32_t i = 0; i < size; i++, p++) {
        if (*p != value) {
            return 0;
        }
    }
    return 1;
}

int64_t satoi(const char *str) {
    if (str == NULL) {
        return 0;
    }

    uint64_t i = 0;
    int64_t res = 0;
    int8_t sign = 1;

    if (str[i] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
        res = res * 10 + (str[i] - '0');
    }

    return res * sign;
}

void bussy_wait(uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
    }
}

void endless_loop(void) {
    while (1) {
    }
}

void endless_loop_print(uint64_t wait) {
    int64_t pid = sys_process_get_pid();

    while (1) {
        printf("%d ", (int)pid);
        bussy_wait(wait);
    }
}
