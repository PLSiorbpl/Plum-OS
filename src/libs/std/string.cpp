#include "string.h"
#include "types.hpp"

namespace std {
    int strcmp (const char *str1, const char *str2) {
        auto s1 = reinterpret_cast<const unsigned char *>(str1);
        auto s2 = reinterpret_cast<const unsigned char *>(str2);
        unsigned char c1, c2;

        do {
            c1 = static_cast<unsigned char>(*s1++);
            c2 = static_cast<unsigned char>(*s2++);
            if (c1 == '\0')
                return c1 - c2;
        } while (c1 == c2);

        return c1 - c2;
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
