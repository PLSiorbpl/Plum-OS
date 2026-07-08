#pragma once
#include "types.hpp"

namespace std {
    class random {
    public:
        explicit random(const uint64_t seed) : m_seed(seed), m_state(seed) {}

        void set_seed(const uint64_t seed) { m_seed = seed; m_state = seed; }
        void reset() { m_state = m_seed; }

        [[nodiscard]] uint64_t get_seed() const { return m_seed; }

        uint64_t iget();
        float fget();
        uint64_t irange(const uint64_t min, const uint64_t max) {
            return min + (iget() % (max - min + 1));
        }

        uint64_t operator()() {
            return iget();
        }
    private:
        uint64_t m_seed;
        uint64_t m_state;
    };

    class perlin2d {
    public:
        perlin2d(const uint64_t seed) : m_seed(seed) {}
        void set_seed(const uint64_t seed) { m_seed = seed; }

        [[nodiscard]] uint64_t get_seed() const { return m_seed; }

        float fget(float x, float y);

    private:
        uint32_t hash(int64_t x, int64_t y);

        uint32_t m_seed;

    };
}
