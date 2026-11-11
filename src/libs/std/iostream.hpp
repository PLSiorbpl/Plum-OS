#pragma once
#include "arch/x86_64/syscall/syscall.h"

namespace std {
    /**
     * @brief reads 1 key
     * @attention No waiting for char
     * @return key_code / KEY_NULL if not pressed
     */
    inline kb::key_code cin() {
        kb::key_code c{};
        sys_read(1, reinterpret_cast<const char *>(&c), 1);
        return c;
    }
}
