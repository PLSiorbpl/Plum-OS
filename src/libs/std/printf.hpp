#pragma once
#include "math_types.hpp"
#include "Drivers/types.h"

namespace std {
    enum class Output {
        std_out = 0,
        std_serial = 1,
    };

    namespace kernel {
        void printf(const char* text, ...);
        void print(const char* text, Color color = Color::LightGray);
        void put_char(char c, Color color = Color::LightGray);
    }

    void printf(const char* text, Output out = Output::std_out, ...);
    void print(const char* text, Output out = Output::std_out, Color color = Color::LightGray);
    void put_char(char c, Output out = Output::std_out, Color color = Color::LightGray);

    void move_cursor(int offset_x, int offset_y);
    void set_cursor(int x, int y);
    glm::ivec2 get_cursor();
    glm::ivec2 get_terminal_size();
}
