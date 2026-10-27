#include "ksyscall.hpp"

#include "kernel/Process/Process_manager.hpp"
#include "kernel/Process/FileDescriptor.hpp"

u64 kwrite(const unsigned int fd, const char *buf, const size_t size) {
    auto process = proc::get_thread();

    if (fd < 0 || fd >= proc::MAX_FDS)
        return -1;

    proc::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->write(buf, size);
}

u64 kread(const unsigned int fd, void *buf, const size_t size) {
    auto process = proc::get_thread();

    if (fd < 0 || fd >= proc::MAX_FDS)
        return -1;

    proc::OpenFile* file = process->fd_table[fd].file;

    if (!file || !file->object)
        return -1;

    return file->object->read(buf, size);
}

u64 kopen(const char *path, const int flags, const int mode) {
    auto process = proc::get_thread();

    proc::FileObject* object = proc::vfs.open(path, flags);
    if (!object)
        return -1;

    proc::OpenFile* file = create_open_file(object, flags);
    if (!file) {
        object->close();
        return -1;
    }

    int fd = alloc_fd(*process, file);
    if (fd < 0) {
        proc::release_open_file(file);
        return -1;
    }

    return fd;
}
