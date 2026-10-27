#pragma once
#include "std/types.hpp"

namespace std {
    bool str_cmp(const char* str1, const char* str2);
    int strlen(const char* text);
    i32 strcpy(char* dst, const char* src);

    template <typename T> struct is_floating_point             { static const bool value = false; };
    template <>           struct is_floating_point<float>      { static const bool value = true;  };
    template <>           struct is_floating_point<double>     { static const bool value = true;  };
    template <>           struct is_floating_point<long double>{ static const bool value = true;  };

    template <typename T>
    static const bool is_floating_point_v = is_floating_point<T>::value;

    template <typename T>
    size_t to_str(char* buffer, T value) {
        if constexpr (is_floating_point_v<T>) {
            // ---------------------------------
            // Floats
            int i = 0;
            bool is_negative = false;

            auto int_part = static_cast<long long>(value);
            double frac_part = value - int_part;

            if (frac_part < 0)
                frac_part = -frac_part;
            constexpr int precision = 4;

            if (int_part < 0) {
                int_part = -int_part;
                is_negative = true;
            }

            if (int_part == 0) {
                buffer[i++] = '0';
            } else {
                while (0 < int_part) {
                    const auto digit = int_part % 10;
                    buffer[i++] = '0' + digit;
                    int_part /= 10;
                }
            }

            if (is_negative) {
                buffer[i++] = '-';
            }

            for (int j = 0; j < i / 2; j++) {
                const char tmp = buffer[j];
                buffer[j] = buffer[i - j - 1];
                buffer[i - j - 1] = tmp;
            }

            // Float part
            buffer[i++] = '.';
            int start_frac = i;
            int last_digit_index = i - 1;

            for (int d = 0; d < precision; d++) {
                frac_part *= 10;
                int digit = static_cast<int>(frac_part);
                buffer[i++] = '0' + digit;
                if (digit != 0) {
                    last_digit_index = i - 1;
                }
                frac_part -= digit;
            }

            if (last_digit_index == start_frac - 1) {
                buffer[start_frac - 1] = '\0';
                return start_frac-1;
            } else {
                buffer[last_digit_index + 1] = '\0';
                return last_digit_index+1;
            }
        } else {
            // ---------------------------------
            // Int
            int i = 0;
            bool is_negative = false;

            if (value < 0) {
                value = -value;
                is_negative = true;
            }

            if (value == 0) {
                buffer[i++] = '0';
            } else {
                while (0 < value) {
                    const auto digit = value % 10;
                    buffer[i++] = '0' + digit;
                    value /= 10;
                }
            }

            if (is_negative) {
                buffer[i++] = '-';
            }

            for (int j = 0; j < i / 2; j++) {
                const char tmp = buffer[j];
                buffer[j] = buffer[i - j - 1];
                buffer[i - j - 1] = tmp;
            }

            buffer[i] = '\0';
            return i;
        }
    }

    template<typename T>
    const char *format_size(T &size) {
        int iters = 0;
        while (size >= 1024) {
            size /= 1024;
            iters++;
        }
        switch (iters) {
            case 0: return "B";
            case 1: return "KB";
            case 2: return "MB";
            case 3: return "GB";
            case 4: return "TB";
            case 5: return "PB";
            case 6: return "WTF";
            default: return "B";
        }
    }

    template<typename T>
    const char* format_number(T &number) {
         int iters = 0;
         while (number >= 1000) {
             number /= 1000;
             iters++;
         }

         switch (iters) {
             case 0: return "K";
             case 1: return "M";
             case 2: return "B";
             case 3: return "T";
             case 4: return "Q";
             case 5: return "Qi";
             case 6: return "SX";
             default: return "";
         }
     }

    template<typename T>
    size_t to_hex_str(char* buffer, T value) {
        buffer[0] = '0';
        buffer[1] = 'x';

        char* ptr = buffer + 2;

        bool started = false;
        auto hex_chars = "0123456789ABCDEF";

        for (int i = (sizeof(T) * 8) - 4; i >= 0; i -= 4) {
            const uint8_t nibble = (value >> i) & 0xF;

            if (!started) {
                if (nibble == 0)
                    continue;
                started = true;
            }

            *ptr++ = hex_chars[nibble];
        }

        if (!started) {
            *ptr++ = '0';
        }

        *ptr = '\0';

        return ptr - buffer;
    }

    inline int64_t str_to_int(const char *str) {
        int result = 0;
        bool negative = false;

        int i = 0;
        if (str[0] == '-') {
            negative = true;
            i++;
        }
        while (str[i]) {
            char c = str[i];

            if (c < '0' || c > '9') {
                break;
            }

            result = result * 10 + (c - '0');
            i++;
        }
        return negative ? -result : result;
    }
}
