#include "vga.h"

#include "cursor.h"
#include "std/mem_common.hpp"
#include "arch/x86_64/Common/Common.hpp"

namespace drivers::vga {
    volatile uint16_t* video = reinterpret_cast<volatile uint16_t* const>(0xB8000);

    void scroll() {
        const bool old_flag = x64::get_INT_flag();
        x64::set_INT_flag(false);
        std::memmove(video, video + 80, 24 * 80 * 2);
        std::memset16(&video[(VGA_HEIGHT-1) * VGA_WIDTH], (static_cast<uint16_t>(Color::LightGray) << 8) | ' ', VGA_WIDTH);
        x64::set_INT_flag(old_flag);
    }

    inline void put_char_at(const char c, const uint8_t x, const uint8_t y, const Color color) {
        const int idx = y * VGA_WIDTH + x;
        video[idx] = static_cast<uint8_t>(color) << 8 | c;
    }

    inline char get_char_at(const uint8_t x, const uint8_t y) {
        const int idx = y * VGA_WIDTH + x;
        return static_cast<char>(video[idx] & 0xFF);
    }

    void put_char(const char c, const Color color) {
        if (c == '\t') { // Tab
            cursor::inc_cursor(TAB_SIZE);
            put_char_at('\t', cursor::cursor_x, cursor::cursor_y, Color::LightGray);
            cursor::update_cursor();
            return;
        }
        if (c == '\n') { // Return
            cursor::cursor_x = 0;
            cursor::cursor_y++;
            if (cursor::cursor_y >= VGA_HEIGHT) {
                scroll();
                cursor::cursor_y = VGA_HEIGHT - 1;
            }

            cursor::update_cursor();
            return;
        }
        if (c == '\b') { // Backspace
            put_char_at(' ', cursor::cursor_x, cursor::cursor_y, Color::LightGray); // just in case
            cursor::dec_cursor(1);
            put_char_at(' ', cursor::cursor_x, cursor::cursor_y, Color::LightGray);
            cursor::update_cursor();
            return;
        }

        put_char_at(c, cursor::cursor_x, cursor::cursor_y, color);

        cursor::inc_cursor(1);
        cursor::update_cursor();
    }

    void print(const char* text, const Color color) {
        for (int i = 0; text[i] != '\0'; i++) {
            put_char(text[i], color);
        }
    }

    void clear(Color color) {
        std::memset16(video, (static_cast<uint8_t>(color) << 8) | ' ', VGA_WIDTH * VGA_HEIGHT);
        cursor::cursor_x = 0;
        cursor::cursor_y = 0;
    }
}
