#include <strings.h>

size_t strlen(const char *str) {
    if (str == NULL) {
        return 0;
    }

    const char *it = str;
    while (*it != '\0') {
        it++;
    }
    return (size_t)(it - str);
}

char *strcpy(char *dest, const char *src) {
    if (dest == NULL || src == NULL) {
        return dest;
    }

    char *out = dest;
    while ((*dest++ = *src++) != '\0') {
        /* copy */
    }
    return out;
}
