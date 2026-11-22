#include "Drivers/fs/ext2/ext2.hpp"
#include "kernel/Memory/heap.hpp"
#include "kernel/Process/fs_types.hpp"

namespace fs::ext2 {
    Ext2File::Ext2File(Ext2& ext2_data, Inode* inode, DirectoryEntry* entry) : inode(inode), ext2_data(ext2_data), position(0), entry(entry) {
        u64 size = inode->size_bytes_lo;
        if (inode->mode.type == InodeType::File)
            size |= static_cast<u64>(inode->size_bytes_hi) << 32;

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

    int Ext2File::getdents64(DirectoryEntry* dirent, unsigned int count) {
        if (count == 0)
            return 0;
        if (!is_directory())
            return -1;

        u64 offset = 0;
        while (position < size()) {
            auto* entry = reinterpret_cast<DirectoryEntry*>(static_cast<u8*>(contents) + position);
            if (entry->entry_size == 0)
                break;
            if (entry->inode != 0) {
                u16 reclen = (offsetof(DirectoryEntry, name) + entry->name_length + 1 + 7) & ~7;
                if (offset + reclen > count)
                    break;

                auto* current = reinterpret_cast<DirectoryEntry*>(reinterpret_cast<u8*>(dirent) + offset);
                memcpy(current, entry, offsetof(DirectoryEntry, name) + entry->name_length);
                current->entry_size = reclen;
                current->name[entry->name_length] = '\0';
                offset += reclen;
            }

            position += entry->entry_size;
        }

        return offset;
    }
};
