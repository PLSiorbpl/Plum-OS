#include "FileDescriptor.hpp"
#include "kernel/log.h"
#include "std/string.h"

namespace vfs {
    bool VFS::mount_matches(const char *path, const char *mount) {
        const size_t len = std::strlen(mount);

        if (std::strncmp(path, mount, len) != 0)
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

        Mount mount{};
        std::strcpy(mount.path, path);
        mount.fs = fs;

        mounts.push_back(mount);
        return true;
    }

    bool VFS::unmount(const char *path) {
        return false;
    }

    Mount *VFS::find_best_mount(const char *path, const char *&relative) {
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

        relative = path + best_len;

        // /dev/stdcout/   ->   stdcout/
        if (best_len == 1 && best->path[0] == '/')
            relative = path + 1;
        else if (*relative == '/')
            relative++;

        return best;
    }

    FileObject* VFS::open(const char *path, const int flags) {
        if (!path || path[0] != '/')
            return nullptr;

        const char *relative = nullptr;
        const Mount *best = find_best_mount(path, relative);

        return best->fs->open(relative, flags);
    }

    int VFS::mkdir(const char *pathname, int mode) {
        if (!pathname || pathname[0] != '/')
            return -1;

        const char *relative = nullptr;
        const Mount *best = find_best_mount(pathname, relative);

        return best->fs->mkdir(relative, mode);
    }
}
