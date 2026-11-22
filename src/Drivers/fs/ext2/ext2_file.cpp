#include "Drivers/fs/ext2/ext2.hpp"
#include "kernel/Memory/heap.hpp"

namespace fs::ext2 {
    Ext2File::Ext2File(Ext2& ext2_data, Inode* inode) : ext2_data(ext2_data), position(0) {
        u64 size = inode->size_bytes_lo;
        if (inode->mode.type == InodeType::File)
            size |= static_cast<u64>(inode->size_bytes_hi) << 32;

        this->inode = inode;
        this->contents = heap::malloc_align(size, 0x1000);
        ext2_data.resolve_inode(*inode, this->contents);
    }

    Ext2File::~Ext2File() {
        heap::free_align(inode);
        heap::free_align(contents);
    }

    int64_t Ext2File::read(void* buffer, size_t size) {
        if (!buffer)
            return -1;

        u64 file_size = this->size();
        if (size > file_size - position)
            size = file_size - position;
        if (position >= file_size)
            return 0;

        memcpy(buffer, static_cast<u8*>(contents) + position, size);
        position += size;
        return size;
    }
};
