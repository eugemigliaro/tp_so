// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <syscalls.h>

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    return sys_mem_alloc(size);
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    sys_mem_free(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    if (nmemb > SIZE_MAX / size) {
        return NULL;
    }

    size_t total = nmemb * size;
    uint8_t *ptr = malloc(total);
    if (ptr == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < total; i++) {
        ptr[i] = 0;
    }

    return ptr;
}

static unsigned long int next = 1;

int rand(void) {

    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % 32768;
}

void srand(unsigned int seed) {
    next = seed;
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    while (str[i] == ' ' || str[i] == '\t') {
        i++;
    }
    
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return sign * result;
}
