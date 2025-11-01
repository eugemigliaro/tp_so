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

// C's stdlib pseudo random number generator
// https://wiki.osdev.org/Random_Number_Generator

static unsigned long int next = 1;  // NB: "unsigned long int" is assumed to be 32 bits wide

int rand(void) { // RAND_MAX assumed to be 32767

    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % 32768;
}

void srand(unsigned int seed) {
    next = seed;
}
