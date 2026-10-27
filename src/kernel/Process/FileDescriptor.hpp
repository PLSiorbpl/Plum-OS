#pragma once
#include "Drivers/Keyboard.hpp"
#include "std/printf.hpp"
#include "std/string.h"
#include "std/types.hpp"
#include "std/vector.hpp"

namespace proc {
    constexpr size_t MAX_FDS = 64;

    class FileObject {
    public:
        virtual ~FileObject() = default;

        virtual int64_t read(void* buffer, size_t size) {
            return -1;
        }

        virtual int64_t write(const void* buffer, size_t size) {
            return -1;
        }

        virtual int64_t seek(int64_t offset, int whence) {
            return -1;
        }

        virtual void close() {}
    };

    struct OpenFile {
        FileObject* object;

        uint64_t position = 0;
        uint32_t flags = 0;

        size_t ref_count = 0;
    };

    struct FDEntry {
        OpenFile* file = nullptr;
        uint32_t flags = 0;
    };

    class stdoutObject : public FileObject {
    public:
        int64_t write(const void *buffer, size_t size) override {
            std::kernel::printf((char *)buffer);
            return size;
        }
    };

    class stdinObject : public FileObject {
    public:
        int64_t read(void *buffer, size_t size) override {
            *static_cast<kb::key_code *>(buffer) = kb::read_char();
            return 1;
        }
    };

    class VFS {
        std::vector<FileObject *> Objects;
    public:
        ~VFS() = default;

        FileObject *open(const char *path, int flags) {
            if (std::str_cmp(path, "stdout")) {
                auto* obj = new stdoutObject();
                Objects.push_back(obj);
                return obj;
            }
            if (std::str_cmp(path, "stdin")) {
                auto* obj = new stdinObject();
                Objects.push_back(obj);
                return obj;
            }

            return nullptr;
        }
    };

    extern VFS vfs;
}
