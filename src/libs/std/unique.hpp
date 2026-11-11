#pragma once
#include "forward.hpp"
#include "utills.hpp"

namespace std {
    template<typename T>
    class unique_ptr {
    private:
        T *ptr;
    public:
        unique_ptr() : ptr(nullptr) {}
        explicit unique_ptr(T *p) : ptr(p) {}

        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;

        unique_ptr(unique_ptr &&src) noexcept : ptr(src.ptr) {
            src.ptr = nullptr;
        }

        template<typename U>
        unique_ptr(unique_ptr<U>&& src) noexcept : ptr(src.get()) {
            src.ptr = nullptr;
        }

        unique_ptr& operator=(unique_ptr &&src) noexcept {
            if (this != &src) {
                delete ptr;
                ptr = src.ptr;
                src.ptr = nullptr;
            }
            return *this;
        }

        template<typename U>
        unique_ptr& operator=(unique_ptr<U>&& src) noexcept {
            delete ptr;
            ptr = src.ptr;
            src.ptr = nullptr;
            return *this;
        }

        ~unique_ptr() {
            delete ptr;
        }

        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }
        T* get() const { return ptr; }

        void reset(T* p = nullptr) {
            delete ptr;
            ptr = p;
        }

        T* release() {
            T* tmp = ptr;
            ptr = nullptr;
            return tmp;
        }

        void swap(unique_ptr& other) noexcept { std::swap(ptr, other.ptr); }

        explicit operator bool() const { return ptr != nullptr; }
        template<typename U> friend class unique_ptr;
    };

    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
