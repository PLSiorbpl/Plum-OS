#pragma once
#include "arch/x86_64/syscall/syscall.h"

namespace std {
    inline kb::key_code cin() {
        kb::key_code c{};
        sys_read(1, reinterpret_cast<const char *>(&c), 1);
        return c;
    }
}
