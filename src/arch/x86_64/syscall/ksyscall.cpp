#include "ksyscall.hpp"

#include "kernel/log.h"
#include "kernel/Process/Process_manager.hpp"
#include "kernel/Process/FileDescriptor.hpp"

u64 kwrite(const unsigned int fd, const char *buf, const size_t size) {
    auto process = vfs::get_thread();

    if (fd < 0 || fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->write(buf, size);
}

u64 kread(const unsigned int fd, void *buf, const size_t size) {
    auto process = vfs::get_thread();

    if (fd < 0 || fd >= vfs::MAX_FDS)
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

u64 kstat(const char *filename, vfs::FileStat *statbuf) {
    vfs::FileObject* object = vfs::vfs.open(filename, 0);
    if (!object)
        return -1;

    const auto ret = object->stat(statbuf);
    object->close();
    return ret;
}

u64 kfstat(unsigned int fd, vfs::FileStat *statbuf) {
    auto process = vfs::get_thread();

    if (fd < 0 || fd >= vfs::MAX_FDS)
        return -1;

    const vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->stat(statbuf);
}

u64 klstat(const char *filename, vfs::FileStat *statbuf) {
    vfs::FileObject* object = vfs::vfs.open(filename, 0);
    if (!object)
        return -1;

    const auto ret = object->stat(statbuf);
    object->close();
    return ret;
}

u64 klseek(const unsigned int fd, const int64_t offset, const unsigned int whence) {
    auto process = vfs::get_thread();

    if (fd < 0 || fd >= vfs::MAX_FDS)
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

    if (fd < 0 || fd >= vfs::MAX_FDS)
        return -1;

    vfs::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->truncate(lenght);
}
