#pragma once
#include "fat32.hpp"
#include "Drivers/achi/ahci_device.h"
#include "kernel/Process/FileDescriptor.hpp"
#include "std/unique.hpp"
#include "kernel/Process/fs_types.hpp"

namespace fs {
    #define EOF 0x0FFFFFFF

    class FAT32FileSystem : public vfs::FileSystem {
        drivers::ahci::ahci_device *device = nullptr;
        FAT32::fat_info fat = {};

        uint64_t root_cluster = 0;
        uint64_t cluster_size = 0;
        uint64_t root_lba = 0;
        uint64_t data_sectors = 0;
        uint64_t max_cluster = 0;

        std::vector<std::unique_ptr<vfs::FileObject>> files;

    public:
        bool mount(drivers::ahci::ahci_device &dev, const FAT32::fat_info& part);
        vfs::FileObject *open(const char *path, int flags) override;

        bool get_next_cluster(uint64_t cluster, uint64_t *next) const;
        bool find_free_cluster(uint64_t *cluster) const;
        bool set_cluster(uint64_t cluster, uint32_t value) const;
        [[nodiscard]] uint64_t calculate_lba(const uint32_t cluster) const {
            return cluster >= 2 ? fat.data_start + static_cast<uint64_t>(cluster - 2) * fat.bpb.sectors_per_cluster : 0;
        }
        static void parse_short_name(char *buf, const FAT32::file_entry *entry);
        static void parse_lfn(char *buf, const FAT32::long_file_name *entry);
        [[nodiscard]] uint64_t get_cluster_size() const { return cluster_size; }
        [[nodiscard]] drivers::ahci::ahci_device *get_dev() const { return device; }
        [[nodiscard]] const FAT32::fat_info *get_fat() const { return &fat; }
        void close(const vfs::FileObject *file);
    };

    class FAT32File : public vfs::FileObject {
        FAT32FileSystem *filesystem = nullptr;
        FAT32::file_entry file = {};
        uint64_t first_cluster;
        uint64_t start_lba = 0;
        uint64_t position = 0;
        uint64_t cluster_size = 0;
        uint64_t directory_entry_offset;
        uint64_t directory_entry_lba;

        bool update_file_entry();

    public:
        FAT32File(FAT32FileSystem *filesystem, const FAT32::file_entry &file, uint64_t directory_entry_offset, uint64_t directory_entry_lba);

        int64_t read(void *buffer, size_t size) override;
        int64_t write(const void *buffer, size_t size) override;
        int64_t seek(int64_t offset, unsigned int whence) override;
        [[nodiscard]] int64_t size() const override { return file.file_size; }
        [[nodiscard]] bool is_directory() const override { return false; }
        int Stat(stat *info) const override;
        int truncate(uint64_t lenght) override;
        void close() override;
    };

    class FAT32Dir : public vfs::FileObject {
        FAT32FileSystem *filesystem = nullptr;
        FAT32::file_entry file = {};
        uint64_t first_cluster;
        uint64_t start_lba = 0;
        uint64_t position = 0;
        uint64_t cluster_size = 0;
        uint64_t directory_entry_offset;
        uint64_t directory_entry_lba;

        bool update_Dir_entry();

    public:
        FAT32Dir(FAT32FileSystem *filesystem, const FAT32::file_entry &file, uint64_t directory_entry_offset, uint64_t directory_entry_lba);

        int getdents64(linux_dirent64 *dirent, unsigned int count) override;
        int64_t seek(int64_t offset, unsigned int whence) override;
        [[nodiscard]] bool is_directory() const override { return true; }
        void close() override;
    };
}
