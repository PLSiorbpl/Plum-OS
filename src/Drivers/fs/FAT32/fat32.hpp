#pragma once
#include "Drivers/achi/ahci_device.h"
#include "Drivers/fs/partition/partition_manager.h"
#include "Data_structures.hpp"

namespace fs {
    class FAT32FileSystem;
}

namespace fs::FAT32 {
    class fat32_manager {
    public:
        fat32_manager() = default;
        ~fat32_manager();
        void init(drivers::ahci::ahci_device &dev);
        static bool validate_fat32(const BPB &bpb);
    private:

        drivers::ahci::ahci_device *device = nullptr;
        partition::partition_manager part_manager;
        std::vector<fat_info> fat_partitions;
        std::vector<FAT32FileSystem> fatfile_systems;
    };
}
