#pragma once
#include "std/types.hpp"
#include "std/vector.hpp"
#include "fs_types.hpp"

namespace vfs {
    class FileObject {
    public:
        virtual ~FileObject() = default;

        virtual int64_t read(void* buffer, size_t size) { return -1; }
        virtual int64_t write(const void* buffer, size_t size) { return -1; }
        virtual int64_t seek(int64_t offset, unsigned int whence) { return -1; }
        virtual int truncate(uint64_t lenght) { return -1; }
        [[nodiscard]] virtual int64_t size() const { return -1; }
        [[nodiscard]] virtual bool is_directory() const { return false; }
        virtual int Stat(stat* info) const { return -1; }
        virtual int getdents64(linux_dirent64 *dirent, unsigned int count) { return -1; }
        virtual void close() {}
    };

    struct OpenFile {
        FileObject* object = nullptr;

        uint64_t position = 0;
        uint32_t flags = 0;

        int32_t ref_count = 0;
    };

    struct FDEntry {
        OpenFile* file = nullptr;
        uint32_t flags = 0;
    };

    class FileSystem {
    public:
        virtual ~FileSystem() = default;
        virtual FileObject* open(const char* path, int flags) = 0;
    };

    struct Mount {
        char path[64];
        FileSystem* fs;
    };

    class VFS {
        std::vector<Mount> mounts;
        static bool mount_matches(const char* path, const char* mount);

    public:
        ~VFS() = default;

        bool mount(const char* path, FileSystem* fs);
        bool unmount(const char* path);
        FileObject *open(const char *path, int flags);
    };

    extern VFS vfs;
}
