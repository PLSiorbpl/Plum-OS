#pragma once
#include "std/types.hpp"

namespace USB {
    struct usb_mouse_state {
        int16_t x = 0;
        int16_t y = 0;
        uint8_t buttons = 0;
        bool updated = false;
    };

    usb_mouse_state mouse_state;
}
