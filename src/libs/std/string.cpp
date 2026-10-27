#include "string.h"
#include "types.hpp"

namespace std {
    bool strcmp(const char *str1, const char *str2) {
        uint64_t idx = 0;
        while (str1[idx] == str2[idx]) {
            if (str1[idx] == '\0') return true;
            idx++;
        }
        return false;
    }

    bool strncmp(const char* str1, const char* str2, uint64_t n) {
        for (uint64_t i = 0; i < n; i++) {
            if (str1[i] != str2[i])
                return false;

            if (str1[i] == '\0')
                return true;
        }

        return true;
    }

    int strlen(const char* text) {
        int len = 0;
        while (text[len] != '\0') {
            len++;
        }
        return len;
    }

    i32 strcpy(char *dst, const char *src) {
        int i = 0;
        while (src[i]) {
            dst[i] = src[i];
            i++;
        }
        dst[i] = '\0';
        return i;
    }
}
