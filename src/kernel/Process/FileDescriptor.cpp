#include "FileDescriptor.hpp"
#include "std/string.h"

namespace vfs {
    bool VFS::mount_matches(const char *path, const char *mount) {
        const size_t len = std::strlen(mount);

        if (!std::strncmp(path, mount, len))
            return false;

        if (len == 1 && mount[0] == '/')
            return path[0] == '/';

        if (path[len] == '\0')
            return true;

        return path[len] == '/';
    }

    bool VFS::mount(const char *path, FileSystem *fs) {
        if (!path || !fs || path[0] != '/')
            return false;

        for (const auto& mount : mounts) {
            if (std::strcmp(mount.path, path) == 0)
                return false;
        }

        mounts.push_back({.path = path, .fs = fs});
        return true;
    }

    bool VFS::unmount(const char *path) {
        return false;
    }

    FileObject *VFS::open(const char *path, const int flags) {
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
}
