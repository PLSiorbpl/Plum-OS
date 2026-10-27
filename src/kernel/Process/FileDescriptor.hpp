#pragma once
#include "Drivers/Keyboard.hpp"
#include "kernel/log.h"
#include "std/printf.hpp"
#include "std/string.h"
#include "std/types.hpp"
#include "std/vector.hpp"

namespace proc {
    constexpr size_t MAX_FDS = 64;

    class FileObject {
    public:
        virtual ~FileObject() = default;

        virtual int64_t read(void* buffer, size_t size) { return -1; }
        virtual int64_t write(const void* buffer, size_t size) { return -1; }
        virtual int64_t seek(int64_t offset, int whence) { return -1; }
        virtual void close() {}
    };

    struct OpenFile {
        FileObject* object = nullptr;

        uint64_t position = 0;
        uint32_t flags = 0;

        uint32_t ref_count = 0;
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

    class FileSystem {
    public:
        virtual ~FileSystem() = default;

        virtual FileObject* open(const char* path, int flags) = 0;
    };

    class DeviceFileSystem : public FileSystem {
        struct DeviceEntry {
            const char* name;
            FileObject* device;
        };

        std::vector<DeviceEntry> devices;

    public:
        bool register_device(const char* name, FileObject* device) {
            if (!name || !device)
                return false;

            for (const auto& entry : devices) {
                if (std::strcmp(entry.name, name) == true)
                    return false;
            }

            devices.push_back({name, device});
            return true;
        }

        FileObject *open(const char *path, int flags) override {
            if (!path)
                return nullptr;

            for (const auto& entry : devices) {
                if (std::strcmp(entry.name, path) == true) {
                    return entry.device;
                }
            }

            return nullptr;
        }

    };

    struct Mount {
        const char* path;
        FileSystem* fs;
    };

    class VFS {
        std::vector<Mount> mounts;

        static bool mount_matches(const char* path, const char* mount) {
            const size_t len = std::strlen(mount);

            if (!std::strncmp(path, mount, len))
                return false;

            if (len == 1 && mount[0] == '/')
                return path[0] == '/';

            if (path[len] == '\0')
                return true;

            return path[len] == '/';
        }
    public:
        ~VFS() = default;

        bool mount(const char* path, FileSystem* fs) {
            if (!path || !fs || path[0] != '/')
                return false;

            for (const auto& mount : mounts) {
                if (std::strcmp(mount.path, path) == true)
                    return false;
            }

            mounts.push_back({.path = path, .fs = fs});
            return true;
        }

        bool unmount(const char* path) {
            // later
            return false;
        }

        FileObject *open(const char *path, int flags) {
            if (!path || path[0] != '/')
                return nullptr;

            //log::info("%s -> ", path);

            Mount* best = nullptr;
            size_t best_len = 0;

            for (auto& m : mounts) {
                const size_t len = std::strlen(m.path);

                if (!mount_matches(path, m.path))
                    continue;

                if (!best || len > best_len) {
                    best = &m;
                    best_len = len;
                }
            }

            if (!best)
                return nullptr;

            const char* relative = path + best_len;

            // /dev/stdcout/   ->   stdcout/
            if (best_len == 1 && best->path[0] == '/')
                relative = path + 1;
            else if (*relative == '/')
                relative++;

            //log::info("%s", relative);

            return best->fs->open(relative, flags);
        }
    };

    extern VFS vfs;
}
