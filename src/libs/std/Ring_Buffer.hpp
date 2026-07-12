#pragma once
#include "types.hpp"

namespace std {
    template<typename T, uint32_t size>
    struct Ring_Buffer {
        T data[size] = {};
        uint32_t head = 0;
        uint32_t tail = 0;

        [[nodiscard]] bool empty() const {
            return tail == head;
        }

        [[nodiscard]] bool full() const {
            return ((head + 1) % size) == tail;
        }

        void clear() {
            for (uint32_t i = 0; i < size; i++)
                data[i] = {};
            head = 0;
            tail = 0;
        }

        bool push(const T& value) {
            const uint32_t next = (head + 1) % size;
            if (next == tail) return false;
            data[head] = value;
            head = next;
            return true;
        }

        bool pop(T& out) {
            if (tail == head) return false;
            out = data[tail];
            tail = (tail + 1) % size;
            return true;
        }
    };
}