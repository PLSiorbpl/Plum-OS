#pragma once
#include "Drivers/achi/ahci_device.h"
#include "Drivers/fs/partition/gpt_partition.h"
#include "Drivers/fs/partition/partition_manager.h"
#include "kernel/Process/FileDescriptor.hpp"
#include "std/types.hpp"

namespace fs::ext2 {
    enum class FilesystemState : u16 {
        Clean = 1,
        Errored = 2,
    };

    enum class ErrorHandlingMode : u16 {
        Ignore = 1,
        Remount = 2,
        Panic = 3,
    };

    enum class OperatingSystemId : u32 {
        Linux = 0,
        GnuHurd = 1,
        Masix = 2,
        FreeBsd = 3,
        Other = 4,
    };

    struct Superblock {
        u32 inode_count;
        u32 block_count;
        u32 su_reserved_blocks;
        u32 unallocated_blocks;
        u32 unallocated_inodes;
        u32 starting_block;
        u32 log2_block_size;
        u32 log2_fragment_size;
        u32 blocks_per_group;
        u32 fragments_per_group;
        u32 inodes_per_group;
        u32 last_mount_time;
        u32 last_written_time;
        u16 mounted_count_before_check;
        u16 mounts_before_check;
        u16 signature;
        FilesystemState state;
        ErrorHandlingMode error_handling_mode;
        u16 version_minor;
        u32 time_of_last_check;
        u32 interval_between_forced_checks;
        OperatingSystemId os_id;
        u32 version_major;
        u16 user_id;
        u16 group_id;
        u32 first_non_reserved_inode;
        u16 inode_size;
        u16 block_group_of_this;
        u32 optional_features_present;
        u32 required_features_present;
        u32 read_only_features;
        u8 file_system_id[16];
        u8 volume_name[16];
        u8 last_mounted_path[64];
        u32 compression_alg_used;
        u8 blocks_to_preallocate_files;
        u8 blocks_to_preallocate_directories;
        u16 unused;
        u8 journal_id[16];
        u32 journal_inode;
        u32 journal_device;
        u32 head_orphan_inode_list;
    };

    struct GroupDescriptor {
        u32 block_usage_map_block_addr;
        u32 inode_usage_map_block_addr;
        u32 inode_table_block_addr;
        u16 unallocated_blocks;
        u16 unallocated_inodes;
        u16 num_directories;
        u8 unused[14];
    };

    enum class InodeType : u16 {
        Fifo = 0x1,
        CharacterDevice = 0x2,
        Directory = 0x4,
        BlockDevice = 0x6,
        File = 0x8,
        SymLink = 0xA,
        Socket = 0xC,
    };

    struct InodePermissions {
        u16 other_execute : 1;
        u16 other_write : 1;
        u16 other_read : 1;
        u16 group_execute : 1;
        u16 group_write : 1;
        u16 group_read : 1;
        u16 user_execute : 1;
        u16 user_write : 1;
        u16 user_read : 1;
        u16 sticky : 1;
        u16 set_group_id : 1;
        u16 set_user_id : 1;
        InodeType type : 4;
    } __attribute__((packed));

    struct Inode {
        InodePermissions mode;
        u16 user_id;
        u32 size_bytes_lo;
        u32 last_access_time;
        u32 creation_time;
        u32 last_modified_time;
        u32 deletion_time;
        u16 group_id;
        u16 hard_links;
        u32 used_disk_sectors;
        u32 flags;
        u32 os_specific_1;
        u32 direct_block_ptrs[12];
        u32 singly_indirect_block_ptr;
        u32 doubly_indirect_block_ptr;
        u32 triply_indirect_block_ptr;
        u32 generation;
        u32 extended_attribute_block;
        u32 size_bytes_hi;
        u32 fragment_block_addr;
        u8 os_specific_2[12];
    } __attribute__((packed));

    class Ext2 : public vfs::FileSystem {
        public:
            void init(drivers::ahci::ahci_device& dev);
            void mount(const char* path);

            Inode* read_inode(u64 num);
            void write_inode(u64 num, Inode& inode);
            void resolve_inode(Inode& inode, void* buffer);

            vfs::FileObject* open(const char *path, int flags) override;

            [[nodiscard]] partition::gpt_partition get_partition() const { return partition; }
            [[nodiscard]] u64 get_size() const { return superblock->block_count * block_size; }

        private:
            bool locate_superblock();
            bool locate_gdt();

            bool read_block(u64 block, void* buffer, u32 count = 0);
            void resolve_block(u32 block, u8 depth, u8* buffer, u64& pos, u64 size);

            drivers::ahci::ahci_device* device = nullptr;
            partition::partition_manager part_manager;
            partition::gpt_partition partition;

            Superblock* superblock = nullptr;
            GroupDescriptor* block_group_descriptor_table = nullptr;
            u32 block_size = 0;
            u32 num_groups = 0;
    };

    class Ext2File : public vfs::FileObject {
        public:
            Ext2File(Ext2& ext2_data, Inode* inode, DirectoryEntry* entry);
            ~Ext2File() override;

            int64_t read(void* buffer, size_t size) override;
            int getdents64(DirectoryEntry* dirent, unsigned int count) override;
            //int64_t write(const void *buffer, size_t size) override;
            //int64_t seek(int64_t offset, unsigned int whence) override;
            [[nodiscard]] int64_t size() const override { return inode->size_bytes_lo | static_cast<u64>(inode->size_bytes_hi) << 32; }
            [[nodiscard]] bool is_directory() const override { return inode->mode.type == InodeType::Directory; }
            //int Stat(stat *info) const override;
            //int truncate(uint64_t lenght) override;
            //void close() override;
        private:
            Inode* inode;
            void* contents;
            Ext2& ext2_data;
            u64 position;
            DirectoryEntry* entry;
    };
}
