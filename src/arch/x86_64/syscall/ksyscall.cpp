#include "ksyscall.hpp"

#include "kernel/log.h"
#include "kernel/Process/Process_manager.hpp"
#include "kernel/Process/FileDescriptor.hpp"

u64 kwrite(const unsigned int fd, const char *buf, const size_t size) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->write(buf, size);
}

u64 kread(const unsigned int fd, void *buf, const size_t size) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->read(buf, size);
}

u64 kopen(const char *path, const int flags, const int mode) {
    auto process = vfs::get_thread();

    vfs::FileObject* object = vfs::vfs.open(path, flags);
    if (!object)
        return -1;

    vfs::OpenFile* file = create_open_file(object, flags);
    if (!file) {
        object->close();
        return -1;
    }

    int fd = alloc_fd(*process, file);
    if (fd < 0) {
        vfs::release_open_file(file);
        return -1;
    }

    return fd;
}

u64 kclose(unsigned int fd) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    vfs::release_fd(*process, fd);
    vfs::release_open_file(file);

    return 0;
}

u64 kstat(const char *filename, stat *statbuf) {
    vfs::FileObject* object = vfs::vfs.open(filename, 0);
    if (!object)
        return -1;

    const auto ret = object->Stat(statbuf);
    object->close();
    return ret;
}

u64 kfstat(unsigned int fd, stat *statbuf) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    const vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->Stat(statbuf);
}

u64 klstat(const char *filename, stat *statbuf) {
    vfs::FileObject* object = vfs::vfs.open(filename, 0);
    if (!object)
        return -1;

    const auto ret = object->Stat(statbuf);
    object->close();
    return ret;
}

u64 klseek(const unsigned int fd, const int64_t offset, const unsigned int whence) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->seek(offset, whence);
}

u64 ktruncate(const char *filename, uint64_t lenght) {
    vfs::FileObject* object = vfs::vfs.open(filename, 0);
    if (!object)
        return -1;

    const auto ret = object->truncate(lenght);
    object->close();
    return ret;
}

u64 kftruncate(unsigned int fd, uint64_t lenght) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->truncate(lenght);
}

u64 kdup(unsigned int fd) {
    auto process = vfs::get_thread();

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    int newfd = alloc_fd(*process, file);
    if (newfd < 0)
        return -1;
    file->ref_count++;
    process->fd_table[newfd].flags = process->fd_table[fd].flags;;

    return newfd;
}

u64 kdup2(unsigned int oldfd, unsigned int newfd) {
    auto process = vfs::get_thread();

    if (oldfd >= vfs::MAX_FDS || newfd >= vfs::MAX_FDS)
        return -1;

    if (oldfd == newfd)
        return oldfd;

    vfs::OpenFile* file = process->fd_table[oldfd].file;

    if (!file || !file->object)
        return -1;

    auto &newfile = process->fd_table[newfd];
    if (newfile.file)
        kclose(newfd);

    file->ref_count++;
    newfile.file = file;
    newfile.flags = process->fd_table[oldfd].flags;

    return newfd;
}

u64 kgetdents64(const unsigned int fd, linux_dirent64 *dirent, const unsigned int count) {
    auto process = vfs::get_thread();

    if (count == 0)
        return 0;
    if (!dirent)
        return -1;

    if (fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->getdents64(dirent, count);
}
