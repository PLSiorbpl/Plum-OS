#pragma once
#include "std/types.hpp"

namespace vfs {
    struct FileStat;
}

u64 kwrite(unsigned int fd, const char* buf, size_t size);
u64 kread(unsigned int fd, void *buf, size_t size);
u64 kopen(const char *path, int flags, int mode);

u64 kstat(const char *filename, vfs::FileStat *statbuf);
u64 kfstat(unsigned int fd, vfs::FileStat *statbuf);
u64 klstat(const char *filename, vfs::FileStat *statbuf);

u64 klseek(unsigned int fd, int64_t offset, unsigned int whence);