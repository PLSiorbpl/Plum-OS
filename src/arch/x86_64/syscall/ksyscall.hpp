#pragma once
#include "std/types.hpp"
#include "kernel/Process/fs_types.hpp"

namespace vfs {
    struct FileStat;
}

u64 kwrite(unsigned int fd, const char* buf, size_t size);
u64 kread(unsigned int fd, void *buf, size_t size);
u64 kopen(const char *path, int flags, int mode);
u64 kclose(unsigned int fd);

u64 kstat(const char *filename, stat *statbuf);
u64 kfstat(unsigned int fd, stat *statbuf);
u64 klstat(const char *filename, stat *statbuf);

u64 klseek(unsigned int fd, int64_t offset, unsigned int whence);

u64 ktruncate(const char *filename, uint64_t lenght);
u64 kftruncate(unsigned int fd, uint64_t lenght);

u64 kdup(unsigned int fd);
u64 kdup2(unsigned int oldfd, unsigned int newfd);

u64 kgetdents64(unsigned int fd, linux_dirent64 *dirent, unsigned int count);