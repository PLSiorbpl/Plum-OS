#include "noise.hpp"
#include "std/math.hpp"

namespace std {
    uint64_t random::iget() {
        m_state += (m_state << 3) - m_seed + 1;
        m_state ^= m_state << 13;
        m_state ^= m_state >> 17;
        m_state ^= m_state << 5;
        return m_state;
    }

    float random::fget() {
        return static_cast<float>(iget()) / 4294967295.0f;
    }

    uint32_t perlin2d::hash(const int64_t x, const int64_t y) {
        uint32_t h = m_seed;

        h ^= static_cast<uint64_t>(x) * 374761393ULL;
        h ^= static_cast<uint64_t>(y) * 668265263ULL;

        h ^= h >> 13;
        h *= 1274126177ULL;
        h ^= h >> 16;

        return h;
    }

    float perlin2d::fget(float x, float y) {
        const int64_t x0 = std::floor(x);
        const int64_t y0 = std::floor(y);
        const int64_t x1 = x0 + 1;
        const int64_t y1 = y0 + 1;

        float fx = x - x0;
        float fy = y - y0;

        fx = fx * fx * (3.0f - 2.0f * fx);
        fy = fy * fy * (3.0f - 2.0f * fy);

        const float a = hash(x0,y0) / 4294967295.0f;
        const float b = hash(x1,y0) / 4294967295.0f;
        const float c = hash(x0,y1) / 4294967295.0f;
        const float d = hash(x1,y1) / 4294967295.0f;

        const float ab = a + (b-a)*fx;
        const float cd = c + (d-c)*fx;

        return ab + (cd-ab)*fy;
    }
}
