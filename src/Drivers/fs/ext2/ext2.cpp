#include "ext2.hpp"
#include "kernel/Memory/heap.hpp"
#include "kernel/log.h"
#include "kernel/system.hpp"
#include "std/types.hpp"

namespace fs::ext2 {
    vfs::FileObject* Ext2::open(const char* path, int flags) {
        auto start = path;
        auto* inode = read_inode(2);
        DirectoryEntry* final_entry = nullptr;
        if (!inode)
            return nullptr;

        while (true) {
            while (*start == '/')
                start++;
            if (*start == '\0')
                break;

            const char* end = std::strchr(start, '/');
            size_t len = end ? static_cast<size_t>(end - start) : std::strlen(start);

            char name[256];
            if (len > 255) {
                delete inode;
                return nullptr;
            }
            memcpy(name, start, len);

            if (inode->mode.type != InodeType::Directory) {
                delete inode;
                return nullptr;
            }

            u64 size = inode->size_bytes_lo;
            u8* buf = new u8[size];
            resolve_inode(*inode, buf);

            u32 child = 0;
            u64 offset = 0;
            while (offset < size) {
                auto* entry = reinterpret_cast<DirectoryEntry*>(buf + offset);
                if (entry->entry_size == 0)
                    break;

                if (entry->inode != 0 && len == entry->name_length && memcmp(entry->name, name, len)) {
                    child = entry->inode;
                    final_entry = entry;
                    break;
                }

                offset += entry->entry_size;
            }

            delete[] buf;
            delete inode;

            if (child == 0)
                return nullptr;

            inode = read_inode(child);
            if (!inode)
                return nullptr;

            if (!end)
                break;
            start = end + 1;
        }

        return new Ext2File(*this, inode, final_entry);
    }

    void Ext2::init(drivers::ahci::ahci_device& dev) {
        if (!dev.is_active()) {
            log::error("[ EXT2 ] Device is inactive!");
            return;
        }

        device = &dev;
        part_manager.init(*device);

        bool found = false;
        for (auto i = 0; i < part_manager.get_partition_count(); i++) {
            if (part_manager.get_partition(i, partition)) {
                if (!locate_superblock() || !locate_gdt()) {
                    continue;
                }

                found = true;
                break;
            }
        }

        if (!found) {
            log::error("[ EXT2 ] No ext2 partition found!");
            return;
        }

        log::success("[ EXT2 ] Initialized ext2 volume: %s", superblock->volume_name);
        auto e = read_inode(2)->creation_time;
        log::success("[ EXT2 ] Root node, can read?: %i", e);
    }

    void Ext2::mount(const char *path) {
        if (!vfs::vfs.mount(path, &systemPL::ext2)) {
            log::error("[ EXT2 ] Failed to mount.");
            return;
        }
        auto size = get_size();
        log::info("[ EXT2 ] &aEXT2 &fpartition: &emount&f=/ext2 &esize&f=%l%s", size, std::format_size(size));
    }

    bool Ext2::locate_superblock() {
        auto* superblock = static_cast<Superblock*>(heap::malloc(1024));
        if (superblock == nullptr) {
            log::error("[ EXT2 ] Failed to allocate superblock!!");
            return false;
        }

        // Superblock always starts at byte 1024
        auto sector_size = device->get_sector_size();
        if (!device->read_bytes(partition.starting_lba + (1024.0 / sector_size), 1024 % sector_size, 1024, reinterpret_cast<u16*>(superblock))) {
            log::error("[ EXT2 ] Failed to get superblock!");
            goto failure;
        }

        if (superblock->signature != 0xEF53) {
            goto failure;
        }

        if(superblock->version_major < 1) {
            log::error("[ EXT2 ] I am too lazy to support versions less than 1.0! :D");
            goto failure;
        }

        this->superblock = superblock;
        this->block_size = 1024 << superblock->log2_block_size;
        this->num_groups = (superblock->block_count + superblock->blocks_per_group - 1 - superblock->starting_block) / superblock->blocks_per_group;
        return true;

        failure:
            heap::free(superblock);
            return false;
    }

    bool Ext2::locate_gdt() {
        u32 gdt_size = this->num_groups * sizeof(GroupDescriptor);
        auto* block_group_descriptor_table = new GroupDescriptor[this->num_groups];
        if (block_group_descriptor_table == nullptr) {
            log::error("[ EXT2 ] Failed to allocate Block Group Descriptor Table!!");
            return false;
        }

        if (!read_block(superblock->starting_block + 1, block_group_descriptor_table, gdt_size)) {
            log::error("[ EXT2 ] Failed to get Block Group Descriptor Table!");
            delete[] block_group_descriptor_table;
            return false;
        }

        if (superblock->state != FilesystemState::Clean) {
            log::warn("[ EXT2 ] Filesystem was not found to be clean. Force fsck!");
        }

        this->block_group_descriptor_table = block_group_descriptor_table;
        return true;
    }

    bool Ext2::read_block(u64 block, void* buffer, u32 count) {
        if (count == 0)
            count = block_size;
        return device->read_bytes(partition.starting_lba, block * block_size, count, reinterpret_cast<u16*>(buffer));
    }

    void Ext2::resolve_inode(Inode& inode, void* buffer) {
        u64 pos = 0;
        u64 size = inode.size_bytes_lo;
        if (inode.mode.type == InodeType::File)
            size |= static_cast<u64>(inode.size_bytes_hi) << 32;

        auto* out = reinterpret_cast<u8*>(buffer);

        for (u8 i = 0; i < 12 && pos < size; i++)
            resolve_block(inode.direct_block_ptrs[i], 0, out, pos, size);

        u32 indirect[3] = {
            inode.singly_indirect_block_ptr,
            inode.doubly_indirect_block_ptr,
            inode.triply_indirect_block_ptr,
        };
        for (u8 depth = 1; depth <= 3 && pos < size; depth++)
            resolve_block(indirect[depth - 1], depth, out, pos, size);
    }

    void Ext2::resolve_block(u32 block, u8 depth, u8* buffer, u64& pos, u64 size) {
        if (pos >= size)
            return;

        if (block == 0) {
            u64 covered = block_size;
            for (u8 i = 0; i < depth; i++)
                covered *= block_size / sizeof(u32);

            u64 len = (size - pos < covered) ? size - pos : covered;
            memset(buffer + pos, 0, len);
            pos += len;
            return;
        }

        if (depth == 0) {
            u32 len = (size - pos < block_size) ? size - pos : block_size;
            if (!read_block(block, buffer + pos, len))
                log::warn("[ EXT2 ] Failed to read a block from inode!");
            pos += len;
            return;
        }

        auto* ptrs = new u32[block_size / sizeof(u32)];
        if (ptrs == nullptr) {
            log::error("[ EXT2 ] Failed to allocate indirect block!");
            return;
        }

        if (!read_block(block, ptrs)) {
            log::warn("[ EXT2 ] Failed to read indirect block %i!", block);
            memset(ptrs, 0, block_size);
        }

        u32 ptr_count = block_size / sizeof(u32);
        for (u32 i = 0; i < ptr_count && pos < size; i++)
            resolve_block(ptrs[i], depth - 1, buffer, pos, size);

        delete[] ptrs;
    }

    Inode* Ext2::read_inode(u64 num) {
        auto inodes_per_group = superblock->inodes_per_group;
        auto index = (num - 1) % inodes_per_group;
        auto group = this->block_group_descriptor_table[(num - 1) / inodes_per_group];

        // cache 1 block latore??
        Inode* inode = new Inode();
        u64 offset = group.inode_table_block_addr * this->block_size + index * superblock->inode_size;
        if (!device->read_bytes(partition.starting_lba, offset, sizeof(Inode), reinterpret_cast<u16*>(inode))) {
            log::error("Failed to read inode #%i!", num);
            delete inode;
            return nullptr;
        }

        return inode;
    }

    void Ext2::write_inode(u64 num, Inode& inode) {
        auto inodes_per_group = superblock->inodes_per_group;
        auto index = (num - 1) % inodes_per_group;
        auto group = this->block_group_descriptor_table[(num - 1) / inodes_per_group];

        u64 offset = group.inode_table_block_addr * this->block_size + index * superblock->inode_size;
        if (!device->write_bytes(partition.starting_lba, offset, sizeof(Inode), reinterpret_cast<u16*>(&inode))) {
            log::error("Failed to write inode #%i!");
        }
    }
}
