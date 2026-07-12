#include "types.hpp"

namespace std {
    extern "C" void* memcpy(void* dst, const void* src, const uint64_t size) {
        auto d64 = static_cast<uint64_t *>(dst);
        auto s64 = static_cast<const uint64_t *>(src);
        uint64_t i = 0;
        for (; i+8 < size; i += 8) {
            *d64++ = *s64++;
        }

        auto d8 = reinterpret_cast<uint8_t *>(d64);
        auto s8 = reinterpret_cast<const uint8_t *>(s64);
        for (; i < size; i++) {
            *d8++ = *s8++;
        }
        return dst;
    }

    extern "C" volatile void* memmove(volatile void* dst, volatile const void* src, const uint64_t size) {
        const auto d = static_cast<volatile uint8_t*>(dst);
        const auto s = static_cast<volatile const uint8_t*>(src);

        if (d < s) {
            for (uint64_t i = 0; i < size; i++)
                d[i] = s[i];
        } else {
            for (uint64_t i = size; i > 0; i--)
                d[i-1] = s[i-1];
        }

        return dst;
    }

    extern "C" void* memset(void* dst, const uint8_t value, const uint64_t size) {
        const auto d = static_cast<uint8_t *>(dst);
        for (uint64_t i = 0; i < size; i++) {
            d[i] = value;
        }
        return dst;
    }

    volatile uint16_t *memset16(volatile uint16_t *dst, const uint16_t value, const uint64_t count) {
        for (uint64_t i = 0; i < count; i++) {
            dst[i] = value;
        }
        return dst;
    }

    uint32_t* memset32(uint32_t* dst, const uint32_t value, const uint64_t count) {
        const uint64_t v = (static_cast<uint64_t>(value) << 32) | value;
        auto* d64 = reinterpret_cast<uint64_t *>(dst);

        uint64_t i = 0;

        for (; i + 2 <= count; i += 2)
            *d64++ = v;

        auto* d32 = reinterpret_cast<uint32_t *>(d64);

        for (; i < count; i++)
            *d32++ = value;

        return dst;
    }

    // True - equal False - not equal
    extern "C" bool memcmp(const void* src1, const void* src2, const uint64_t size) {
        const auto p1 = static_cast<const uint8_t*>(src1);
        const auto p2 = static_cast<const uint8_t*>(src2);

        for (size_t i = 0; i < size; i++) {
            if (p1[i] != p2[i])
                return false;
        }

        return true;
    }
}
