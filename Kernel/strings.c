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

int strcmp(const char *str1, const char *str2) {
	int i = 0;
	while (str1[i] != 0 && str2[i] != 0) {
		if (str1[i] != str2[i]) {
			return str1[i] - str2[i];
		}
		i++;
	}
	return str1[i] - str2[i];
}