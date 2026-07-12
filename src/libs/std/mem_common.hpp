#pragma once
#include "types.hpp"

namespace std {
    template <typename T>
    constexpr T&& move(T& t) noexcept {
        return static_cast<T&&>(t);
    }
}

namespace std {
    extern "C" void* memcpy(void* dst, const void* src, uint64_t size);
    extern "C" volatile void* memmove(volatile void* dst, volatile const void* src, uint64_t size);

    extern "C" void* memset(void* dst, uint8_t value, uint64_t size);

    volatile uint16_t *memset16(volatile uint16_t *dst, uint16_t value, uint64_t count);
    uint32_t *memset32(uint32_t *dst, uint32_t value, uint64_t count);

    extern "C" bool memcmp(const void* src1, const void* src2, uint64_t size);
}