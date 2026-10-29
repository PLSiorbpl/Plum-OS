#pragma once
#include "std/types.hpp"
#include "std/vector.hpp"

enum class SeekMode : uint8_t {
    SEEK_SET = 0,
    SEEK_CUR = 1,
    SEEK_END = 2,
};

namespace vfs {
    constexpr size_t MAX_FDS = 64;

    enum class FileType : uint8_t {
        Unknown,
        Regular,
        Directory,
        Device,
        Pipe,
        Symlink
    };

    struct FileStat {
        uint64_t size;
        uint64_t created_time;
        uint64_t modified_time;
        uint32_t attributes;
        uint8_t type;
    };

    class FileObject {
    public:
        virtual ~FileObject() = default;

        virtual int64_t read(void* buffer, size_t size) { return -1; }
        virtual int64_t write(const void* buffer, size_t size) { return -1; }
        virtual int64_t seek(int64_t offset, unsigned int whence) { return -1; }
        [[nodiscard]] virtual int64_t size() const { return -1; }
        [[nodiscard]] virtual bool is_directory() const { return false; }
        virtual int stat(FileStat* info) const { return -1; }
        virtual void close() {}
    };

    struct BlockDevice {
        const char* name;
        FileObject* device;
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

    class FileSystem {
    public:
        virtual ~FileSystem() = default;
        virtual FileObject* open(const char* path, int flags) = 0;
    };

    struct Mount {
        const char* path;
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
