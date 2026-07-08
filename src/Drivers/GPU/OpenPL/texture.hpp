#pragma once
#include "std/types.hpp"
#include "std/math.hpp"
#include "std/math_types.hpp"

namespace OpenPL {
    struct Texture2D {
        uint8_t *texture;
        uint16_t width;
        uint16_t height;

        [[nodiscard]] glm::vec3 getf(const float u, const float v) const {
            const int tx = std::clamp(static_cast<int>(u * width), 0, width-1);
            const int ty = std::clamp(static_cast<int>(v * height), 0, height-1);

            const auto p = &texture[(ty * width + tx) * 3];
            return {p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f};
        }

        [[nodiscard]] uint32_t geti(const float u, const float v) const {
            const int tx = std::clamp(static_cast<int>(u * width), 0, width-1);
            const int ty = std::clamp(static_cast<int>(v * height), 0, height-1);

            const auto p = &texture[(ty * width + tx) * 3];
            return (static_cast<uint32_t>(p[0]) << 16) | (static_cast<uint32_t>(p[1]) << 8) | static_cast<uint32_t>(p[2]);
        }
    };
}
