#ifndef _LIBC_STDLIB_H_
#define _LIBC_STDLIB_H_

#include <stddef.h>

int rand(void);

void srand(unsigned int seed);

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

int atoi(const char *str);

#endif
