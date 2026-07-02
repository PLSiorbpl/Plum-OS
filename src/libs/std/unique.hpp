#pragma once

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

        unique_ptr& operator=(unique_ptr &&src) noexcept {
            if (this != &src) {
                delete ptr;
                ptr = src.ptr;
                src.ptr = nullptr;
            }
            return *this;
        }

        ~unique_ptr() {
            delete ptr;
        }

        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }
        T* get() const { return ptr; }

        explicit operator bool() const { return ptr != nullptr; }
    };

    template<typename T>
    unique_ptr<T> make_unique() {
        return unique_ptr<T>(new T());
    }
}
