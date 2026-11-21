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

    int strncmp(const char *str1, const char *str2, size_t n) {
        for (size_t i = 0; i < n; i++) {
            const auto a = static_cast<unsigned char>(str1[i]);
            const auto b = static_cast<unsigned char>(str2[i]);
            if (a != b)
                return static_cast<int>(a) - static_cast<int>(b);  // <0, 0, >0
            if (a == '\0')
                return 0;
        }
        return 0;
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

    char *strchr(const char *text, char c) {
        if (!text) return nullptr;
        int i = 0;
        while (text[i] != '\0') {
            if (text[i] == c)
                return (char *)&text[i];
            i++;
        }
        return nullptr;
    }

    char *strrchr(const char *str, int ch) {
        const char *last = nullptr;
        const char c = static_cast<char>(ch);

        while (*str) {
            if (*str == c)
                last = str;
            str++;
        }

        if (*str == c)
            last = str;

        return (char *)last;
    }
}
