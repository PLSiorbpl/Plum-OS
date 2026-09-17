#pragma once
#include "std/types.hpp"
#include "std/math_types.hpp"

namespace hash {
    template<typename T>
    struct hash;

    template<>
    struct hash<int> {
        size_t operator()(const int x) const {
            return static_cast<size_t>(x);
        }
    };

    template<>
    struct hash<uint64_t> {
        size_t operator()(const uint64_t x) const {
            return static_cast<size_t>(x);
        }
    };

    template<>
    struct hash<glm::ivec2> {
        size_t operator()(const glm::ivec2 x) const {
            return static_cast<uint64_t>(x.x) << 32 | static_cast<uint32_t>(x.y);
        }
    };
}
