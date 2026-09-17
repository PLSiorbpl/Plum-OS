#include "printf.hpp"

#include "string.h"
#include "arch/x86_64/syscall/syscall.h"
#include "Drivers/vga.h"
#include "Drivers/GPU/framebuffer.hpp"
#include "kernel/system.hpp"

namespace std {
    namespace kernel {
        void printf(const char* text, ...) {
            __builtin_va_list args;
            __builtin_va_start(args, text);

            auto active_color = Color::White;
            for (int i = 0; text[i] != '\0'; i++) {
                const char c = text[i];

                char num_buf[16];

                if (c == '%') {
                    const char arg = text[i + 1];
                    switch (arg) {
                        case 'c':
                            put_char(static_cast<char>(__builtin_va_arg(args, int)), active_color);
                            break;
                        case 's':
                            print(__builtin_va_arg(args, const char*), active_color);
                            break;
                        case 'l':
                            std::to_str(num_buf, __builtin_va_arg(args, long long));
                            print(num_buf, active_color);
                            break;
                        case 'i':
                        case 'd':
                            std::to_str(num_buf, __builtin_va_arg(args, int));
                            print(num_buf, active_color);
                            break;
                        case 'u':
                            std::to_str(num_buf, __builtin_va_arg(args, unsigned int));
                            print(num_buf, active_color);
                            break;
                        case 'x':
                            std::to_hex_str(num_buf, __builtin_va_arg(args, unsigned long long));
                            print(num_buf, active_color);
                            break;
                        case 'f':
                            std::to_str(num_buf, __builtin_va_arg(args, double));
                            print(num_buf, active_color);
                            break;

                        default: {
                            put_char(c, active_color);
                            continue;
                        }
                    }

                    i++;
                    continue;
                }

                if (c == '&') {
                    const char arg = text[i + 1];
                    switch (arg) {
                        case '0':
                            active_color = Color::Black;
                            break;
                        case '1':
                            active_color = Color::Blue;
                            break;
                        case '2':
                            active_color = Color::Green;
                            break;
                        case '3':
                            active_color = Color::Cyan;
                            break;
                        case '4':
                            active_color = Color::Red;
                            break;
                        case '5':
                            active_color = Color::Magenta;
                            break;
                        case '6':
                            active_color = Color::Brown;
                            break;
                        case '7':
                            active_color = Color::LightGray;
                            break;
                        case '8':
                            active_color = Color::DarkGray;
                            break;
                        case '9':
                            active_color = Color::LightBlue;
                            break;
                        case 'a':
                            active_color = Color::LightGreen;
                            break;
                        case 'b':
                            active_color = Color::LightCyan;
                            break;
                        case 'c':
                            active_color = Color::LightRed;
                            break;
                        case 'd':
                            active_color = Color::Pink;
                            break;
                        case 'e':
                            active_color = Color::Yellow;
                            break;
                        case 'f':
                            active_color = Color::White;
                            break;

                        default: {
                            put_char(c, active_color);
                            continue;
                        }
                    }

                    i++;
                    continue;
                }

                put_char(c, active_color);
            }

            __builtin_va_end(args);
        }

        void print(const char *text, const Color color) {
            for (int i = 0; text[i] != '\0'; i++) {
                systemPL::fb.put_char(text[i], color_to_rgb(color));
            }
        }

        void put_char(const char c, const Color color) {
            //drivers::vga::put_char(c, color);
            systemPL::fb.put_char(c, color_to_rgb(color));
        }
    }

    void printf(const char* text, const Output out, ...) {
        __builtin_va_list args;
        __builtin_va_start(args, out);

        auto active_color = Color::White;
        for (int i = 0; text[i] != '\0'; i++) {
            const char c = text[i];

            char num_buf[16];

            if (c == '%') {
                const char arg = text[i + 1];
                switch (arg) {
                    case 'c':
                        std::put_char(static_cast<char>(__builtin_va_arg(args, int)), out, active_color);
                        break;
                    case 's':
                        std::print(__builtin_va_arg(args, const char*), out, active_color);
                        break;
                    case 'l':
                        std::to_str(num_buf, __builtin_va_arg(args, long long));
                        std::print(num_buf, out, active_color);
                        break;
                    case 'i':
                    case 'd':
                        std::to_str(num_buf, __builtin_va_arg(args, int));
                        std::print(num_buf, out, active_color);
                        break;
                    case 'u':
                        std::to_str(num_buf, __builtin_va_arg(args, unsigned int));
                        std::print(num_buf, out, active_color);
                        break;
                    case 'x':
                        std::to_hex_str(num_buf, __builtin_va_arg(args, unsigned long long));
                        std::print(num_buf, out, active_color);
                        break;
                    case 'f':
                        std::to_str(num_buf, __builtin_va_arg(args, double));
                        std::print(num_buf, out, active_color);
                        break;

                    default: {
                        std::put_char(c, out, active_color);
                        continue;
                    }
                }

                i++;
                continue;
            }

            if (c == '&') {
                const char arg = text[i + 1];
                switch (arg) {
                    case '0':
                        active_color = Color::Black;
                        break;
                    case '1':
                        active_color = Color::Blue;
                        break;
                    case '2':
                        active_color = Color::Green;
                        break;
                    case '3':
                        active_color = Color::Cyan;
                        break;
                    case '4':
                        active_color = Color::Red;
                        break;
                    case '5':
                        active_color = Color::Magenta;
                        break;
                    case '6':
                        active_color = Color::Brown;
                        break;
                    case '7':
                        active_color = Color::LightGray;
                        break;
                    case '8':
                        active_color = Color::DarkGray;
                        break;
                    case '9':
                        active_color = Color::LightBlue;
                        break;
                    case 'a':
                        active_color = Color::LightGreen;
                        break;
                    case 'b':
                        active_color = Color::LightCyan;
                        break;
                    case 'c':
                        active_color = Color::LightRed;
                        break;
                    case 'd':
                        active_color = Color::Pink;
                        break;
                    case 'e':
                        active_color = Color::Yellow;
                        break;
                    case 'f':
                        active_color = Color::White;
                        break;

                    default: {
                        std::put_char(c, out, active_color);
                        continue;
                    }
                }

                i++;
                continue;
            }

            std::put_char(c, out, active_color);
        }

        __builtin_va_end(args);
    }

    void print(const char* text, const Output out, const Color color) {
        switch (out) {
            case Output::std_out:
                sys_write(text, static_cast<u64>(color));
                break;
            case Output::std_serial:
                sys_serial_write(text);
                break;
        }
    }

    void put_char(const char c, const Output out, const Color color) {
        switch (out) {
            case Output::std_out:
                sys_put_char(c, static_cast<u64>(color));
                break;
            case Output::std_serial:
                sys_serial_put_char(c);
                break;
        }
    }

    void move_cursor(const int offset_x, const int offset_y) {
        systemPL::fb.move_cursor(offset_x, offset_y);
    }
    void set_cursor(const int x, const int y) {
        systemPL::fb.set_cursor(x, y);
    }
    glm::ivec2 get_cursor() {
        return systemPL::fb.get_cursor();
    }
    glm::ivec2 get_terminal_size() {
        return systemPL::fb.get_terminal_size();
    }
}
