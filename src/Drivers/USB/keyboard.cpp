#include "keyboard.hpp"
#include "std/types.hpp"
#include "Drivers/Keyboard.hpp"

namespace USB {
    usb_keyboard_state kb_state = {};

    void kb_push_key(const uint8_t *report) {
        uint8_t* prev = kb_state.keys;

        for (int k = 0; k < 6; k++) {
            uint8_t hid_key = report[2 + k];
            if (hid_key == 0x00) continue;

            bool was_pressed = false;
            for (int p = 0; p < 6; p++) {
                if (prev[p] == hid_key) { was_pressed = true; break; }
            }

            if (!was_pressed) {
                kb::buf.push(hid_to_ps2[hid_key]);
            }
        }
        for (int k = 0; k < 6; k++)
            kb_state.keys[k] = report[2+k];
    }
}
