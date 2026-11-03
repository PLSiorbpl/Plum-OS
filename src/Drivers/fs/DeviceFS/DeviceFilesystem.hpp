#pragma once
#include "Drivers/Keyboard.hpp"
#include "std/string.h"
#include "kernel/Process/FileDescriptor.hpp"
#include "std/printf.hpp"
#include "std/unique.hpp"

namespace fs {
    class DeviceFileSystem : public vfs::FileSystem {
        std::vector<std::unique_ptr<vfs::BlockDevice>> devices;

    public:
        bool register_device(const char* name, vfs::FileObject* device);

        vfs::FileObject *open(const char *path, int flags) override;
        void close(const vfs::FileObject *file);
    };

    class stdoutObject : public vfs::FileObject {
    public:
        int64_t write(const void *buffer, size_t size) override {
            std::kernel::printf((char *)buffer);
            return size;
        }
    };

    class stdinObject : public vfs::FileObject {
    public:
        int64_t read(void *buffer, size_t size) override {
            *static_cast<kb::key_code *>(buffer) = kb::read_char();
            return 1;
        }
    };
}
