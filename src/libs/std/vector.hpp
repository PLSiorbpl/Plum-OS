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
            m_capacity = 0;
            m_size = 0;
            m_data = nullptr;
        }

        vector(vector&& other) noexcept : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        vector& operator=(vector&& other) noexcept {
            if (this != &other) {
                for (uint64_t i = 0; i < m_size; i++)
                    m_data[i].~T();
                heap::free(m_data);

                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                other.m_data = nullptr;
                other.m_size = 0;
                other.m_capacity = 0;
            }
            return *this;
        }

        vector(const vector&) = delete;
        vector& operator=(const vector&) = delete;

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

        void erase(size_t index) {
            for (size_t i = index; i < m_size - 1; i++)
                m_data[i] = m_data[i + 1];
            m_size--;
        }

        void shrink_to_fit() {
            if (m_size == m_capacity)
                return;

            if (m_size == 0)
            {
                heap::free(m_data);
                m_data = nullptr;
                m_capacity = 0;
                return;
            }

            reserve(m_size);
        }

        void release() {
            clear();
            heap::free(m_data);
            m_data = nullptr;
            m_capacity = 0;
        }

        void reserve(const uint64_t new_size) {
            if (new_size <= m_capacity) return;

            T *old_data = m_data;
            m_data = static_cast<T*>(heap::malloc(sizeof(T) * new_size));

            for (uint64_t i = 0; i < m_size; i++) {
                new (&m_data[i]) T(std::move(old_data[i]));
                old_data[i].~T();
            }

            m_capacity = new_size;
            heap::free(old_data);
        }

        void push_back(T x) {
            if (m_size == m_capacity) {
                reserve(m_capacity == 0 ? 1 : m_capacity * 2);
            }
            new (&m_data[m_size]) T(std::move(x));
            m_size++;
        }

        void pop_back() {
            if (m_size == 0)
                return;
            m_size--;
            m_data[m_size].~T();
        }

        T& back() {
            return m_data[m_size - 1];
        }

        const T& back() const {
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