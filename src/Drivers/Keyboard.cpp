#include "Keyboard.hpp"

#include "kernel/system.hpp"

namespace kb {
    std::Ring_Buffer<uint8_t, 512> buf = {};

    /**
     * @brief Reads key_code from buffer
     * @attention Waits for key forever
     * @return key_code
     * @todo Add wait limit
     */
    key_code get_char() {
        while (buf.empty()) {
            systemPL::fb.swap();
            asm volatile("sti; hlt; cli");
        }

        uint8_t sc;
        buf.pop(sc);
        return static_cast<key_code>(sc);
    }

    /**
     * @brief Reads key_code from buffer
     * @attention No waiting for key
     * @return key_code / KEY_NULL if nothing pressed
     */
    key_code read_char() {
        if (buf.empty())
            return key_code::KEY_NULL;

        uint8_t sc;
        buf.pop(sc);
        buf.clear();
        return static_cast<key_code>(sc);
    }

    /**
     * @brief Converts from key_code to char
     * @param code key_code
     * @return char corresponding to key_code
     */
    char to_char(key_code code) {
        return ascii_map[static_cast<uint8_t>(code)];
    }
}
