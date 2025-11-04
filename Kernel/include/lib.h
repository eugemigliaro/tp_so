#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <stddef.h>
#include <fonts.h>

#define EOF -1

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
void printf(const char * string);
uint32_t uint_to_base(uint64_t value, char *buffer, uint32_t base);

uint8_t getKeyboardBuffer(void);
uint8_t getKeyboardStatus(void);

uint8_t getSecond(void);
uint8_t getMinute(void);
uint8_t getHour(void);

uint8_t * stackInit(void * rsp, void * rip, int argc, char ** argv);

void semLock(uint8_t *lock);
void semUnlock(uint8_t *lock);

int32_t print_mem_status_common(size_t total, size_t used, size_t available);
#endif
