#include "fat32.hpp"

#include "FAT32FileSystem.hpp"
#include "Drivers/fs/partition/gpt_partition.h"
#include "Drivers/fs/partition/partition_manager.h"
#include "kernel/log.h"

namespace fs::FAT32 {
    fat32_manager::~fat32_manager() {
        if (fat_partitions.empty())
            return;

        for (auto & fat_partition : fat_partitions) {
            heap::free_align(&fat_partition);
        }
    }

    void fat32_manager::init(drivers::ahci::ahci_device &dev) {
        if (!dev.is_active()) {
            log::error("[ FAT32 ] Device is inactive");
            return;
        }
        fat_partitions.clear();
        device = &dev;
        part_manager.init(*device);

        for (size_t i = 0; i < part_manager.get_partition_count(); i++) {
            const auto fat = static_cast<fat_info *>(heap::malloc_align(sizeof(fat_info), 0x1000));

            // Get parition candidate
            if (!part_manager.get_partition(i, fat->gpt_partition)) {
                heap::free_align(fat);
                log::warn("[ FAT32 ] Failed to get partition: #%u", i);
                continue;
            }

            // Read its BPB
            if (!device->read_bytes(fat->gpt_partition.starting_lba, 0, sizeof(BPB), reinterpret_cast<u16 *>(&fat->bpb))) {
                heap::free_align(fat);
                log::warn("[ FAT32 ] Failed to read BPB: lba=%l size=%u", static_cast<uint64_t>(fat->gpt_partition.starting_lba), sizeof(BPB));
                continue;
            }

            const uint32_t data_start = fat->gpt_partition.starting_lba + fat->bpb.rsvd_sector_count + fat->bpb.num_of_FATs * fat->bpb.ebpb.sectors_per_FAT;
            fat->data_start = data_start;

            // Validate FAT32
            if (validate_fat32(fat->bpb)) {
                fat_partitions.push_back(*fat);

                fatfile_systems.push_back(FAT32FileSystem());
                FAT32FileSystem *fat32_file_system = &fatfile_systems.back();
                fat32_file_system->mount(*device, *fat);
                //vfs::vfs.mount("/", fat32_file_system);

                char oem_name[9];
                for (int s = 0; s < 8; s++) {
                    oem_name[s] = fat->bpb.OEM_Name[s];
                }
                oem_name[8] = '\0';

                char volume_string[12];
                for (int s = 0; s < 11; s++) {
                    volume_string[s] = static_cast<char>(fat->bpb.ebpb.volume_label_string[s]);
                }
                volume_string[11] = '\0';
            }

            heap::free_align(fat);
        }
    }

    void fat32_manager::mount() {
        vfs::FileObject *file = nullptr;
        FAT32FileSystem *root_fs32 = nullptr;

        for (auto &fat32_fs : fatfile_systems) {
            file = fat32_fs.open("fstab", 0);
            if (file) {
                auto fat = fat32_fs.get_fat();
                auto size = (u64)fat->bpb.large_sector_count * (u64)fat->bpb.bytes_per_sector;
                file->close();

                vfs::vfs.mount("/", &fat32_fs);
                root_fs32 = &fat32_fs;
                vfs::vfs.mount("/", &fat32_fs);
                log::info("&aFAT32 &fpartition: &eMount&f=/ &esize&f=%l%s", size, std::format_size(size));
                break;
            }
        }

        uint8_t i = 0;
        for (auto &fat32_fs : fatfile_systems) {
            if (root_fs32 == &fat32_fs)
                continue;
            auto fat = fat32_fs.get_fat();
            auto size = (u64)fat->bpb.large_sector_count * (u64)fat->bpb.bytes_per_sector;

            char path[] = "/mnt/fat32/ ";
            path[11] = '0' + (char)i;

            vfs::vfs.mount(path, &fat32_fs);
            log::info("&aFAT32 &fpartition: &eMount&f=%s &esize&f=%l%s", path, size, std::format_size(size));
            i++;
        }
    }

    bool fat32_manager::validate_fat32(const BPB& bpb) {
        if (bpb.ebpb.boot_partition_signature != 0xAA55)
            return false;

        // bytes_per_sector must be a power of 2 between 512-4096
        if (bpb.bytes_per_sector != 512  && bpb.bytes_per_sector != 1024 &&
            bpb.bytes_per_sector != 2048 && bpb.bytes_per_sector != 4096)
            return false;

        // sectors_per_cluster must be non-zero power of 2
        if (bpb.sectors_per_cluster == 0)
            return false;
        if (bpb.sectors_per_cluster & (bpb.sectors_per_cluster - 1))
            return false;

        // FAT32 specific: root_entry_count must be 0, FAT_size must be 0
        if (bpb.root_entry_count != 0 || bpb.FAT_size != 0)
            return false;

        // Reserved sector count must be non-zero (holds the BPB itself)
        if (bpb.rsvd_sector_count == 0)
            return false;

        return true;
    }
}
