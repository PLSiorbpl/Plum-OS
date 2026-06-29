#pragma once
#include "types.hpp"
#include "kernel/Memory/heap.hpp"
#include "std/mem_common.hpp"

namespace std {
    template<typename T>
    struct vector {
        T *m_data = nullptr;
        uint64_t m_size = 0;
        uint64_t m_capacity = 0;

        vector() = default;
        ~vector() {
            for (uint64_t i = 0; i < m_size; i++)
                m_data[i].~T();
            heap::free(m_data);
        }

        // Helpers
        [[nodiscard]] uint64_t capacity() const {
            return m_capacity;
        }

        [[nodiscard]] uint64_t size() const {
            return m_size;
        }

        [[nodiscard]] bool empty() const {
            return m_size == 0;
        }

        [[nodiscard]] T* begin() const {
            return m_data;
        }

        [[nodiscard]] T* end() const {
            return m_data + m_size;
        }

        void clear() {
            for (uint64_t i = 0; i < m_size; i++)
                m_data[i].~T();
            m_size = 0;
        }

        void reserve(const uint64_t new_size) {
            if (new_size <= m_capacity) return;

            T *old_data = m_data;
            m_data = static_cast<T*>(heap::malloc(sizeof(T) * new_size));

            for (uint64_t i = 0; i < m_size; i++) {
                new (&m_data[i]) T(old_data[i]);
                old_data[i].~T();
            }

            m_capacity = new_size;
            heap::free(old_data);
        }

        void push_back(const T &x) {
            if (m_size == m_capacity) {
                reserve(m_capacity == 0 ? 1 : m_capacity * 2);
            }
            new (&m_data[m_size]) T(x);
            m_size++;
        }

        T pop_back() {
            m_size--;
            T ret = m_data[m_size];
            m_data[m_size].~T();
            return ret;
        }

        T& back() {
            return m_data[m_size - 1];
        }

        T& operator[](size_t index) {
            return m_data[index];
        }

        const T& operator[](size_t index) const {
            return m_data[index];
        }
    };
}