// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stddef.h>
#include <lib.h>

void * memset(void * destination, int32_t c, uint64_t length)
{
	uint8_t chr = (uint8_t)c;
	char * dst = (char*)destination;

	while(length--)
		dst[length] = chr;

	return destination;
}

void * memcpy(void * destination, const void * source, uint64_t length)
{
	/*
	* memcpy does not support overlapping buffers, so always do it
	* forwards. (Don't change this without adjusting memmove.)
	*
	* For speedy copying, optimize the common case where both pointers
	* and the length are word-aligned, and copy word-at-a-time instead
	* of byte-at-a-time. Otherwise, copy by bytes.
	*
	* The alignment logic below should be portable. We rely on
	* the compiler to be reasonably intelligent about optimizing
	* the divides and modulos out. Fortunately, it is.
	*/
	uint64_t i;

	if ((uint64_t)destination % sizeof(uint32_t) == 0 &&
		(uint64_t)source % sizeof(uint32_t) == 0 &&
		length % sizeof(uint32_t) == 0)
	{
		uint32_t *d = (uint32_t *) destination;
		const uint32_t *s = (const uint32_t *)source;

		for (i = 0; i < length / sizeof(uint32_t); i++)
			d[i] = s[i];
	}
	else
	{
		uint8_t * d = (uint8_t*)destination;
		const uint8_t * s = (const uint8_t*)source;

		for (i = 0; i < length; i++)
			d[i] = s[i];
	}

	return destination;
}

uint32_t uint_to_base(uint64_t value, char *buffer, uint32_t base)
{
	const char symbols[] = "0123456789ABCDEF";
	uint32_t i = 0;

	if (buffer == NULL || base < 2 || base > 16) {
		return 0;
	}

	if (value == 0) {
		buffer[i++] = '0';
		return i;
	}

	while (value != 0) {
		buffer[i++] = symbols[value % base];
		value /= base;
	}

	for (uint32_t j = 0; j < i / 2; j++) {
		char tmp = buffer[j];
		buffer[j] = buffer[i - j - 1];
		buffer[i - j - 1] = tmp;
	}

	return i;
}

int32_t print_mem_status_common(size_t total, size_t used, size_t available) {
    print("=== Memory Status ===\n");
    print("Total: ");
    printDec((uint64_t)total);
    newLine();
    print("Used:  ");
    printDec((uint64_t)used);
    newLine();
    print("Free:  ");
    printDec((uint64_t)available);
    newLine();
    return 0;
}
