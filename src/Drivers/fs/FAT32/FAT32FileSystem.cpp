#include "FAT32FileSystem.hpp"
#include "kernel/log.h"

using namespace vfs;

namespace fs {
    bool FAT32FileSystem::mount(drivers::ahci::ahci_device &dev, const FAT32::fat_info &part) {
        device = &dev;
        fat = part;

        root_cluster = fat.bpb.ebpb.root_directory_cluster;
        cluster_size = fat.bpb.bytes_per_sector * fat.bpb.sectors_per_cluster;
        root_lba = calculate_lba(root_cluster);

        return true;
    }

    FileObject *FAT32FileSystem::open(const char *path, int flags) {
        auto *buffer = static_cast<FAT32::file_entry *>(heap::malloc(cluster_size));

        if (!device->read_bytes(root_lba, cluster_size, reinterpret_cast<uint16_t *>(buffer))) {
            heap::free(buffer);
            log::error("[ FAT32 ] Failed to read cluster: lba=%l cluster_size=%u", root_lba, cluster_size);
            return nullptr;
        }

        char long_name[255] = {};
        const auto *entry = buffer;
        for (size_t i = 0; i < cluster_size/sizeof(FAT32::file_entry); i++) {
            char short_name[12];
            for (int n = 0; n < 11; n++) {
                short_name[n] = entry[i].file_name[n];
            }
            short_name[11] = '\0';

            if (entry[i].file_name[0] == 0x0)
                break;
            if (static_cast<uint8_t>(entry[i].file_name[0]) == 0xE5) // Deleted file
                continue;

            // Long file name
            if (entry[i].attributes == 0x0F) {
                auto *l_name = (FAT32::long_file_name *)&entry[i];
                uint8_t idx = ((l_name->order & 0x1F) - 1) * 13;
                if (idx >= 255) {
                    long_name[254] = '\0';
                    continue;
                }

                for (int id = 0; id < 5; id++) {
                    if (l_name->name_1[id] == 0xFFFF) // garbage
                        continue;
                    if (l_name->name_1[id] == 0x0000) { // end of name
                        long_name[idx] = '\0';
                        break;
                    }

                    long_name[idx] = static_cast<char>(l_name->name_1[id]);
                    idx++;
                }

                for (int id = 0; id < 6; id++) {
                    if (l_name->name_2[id] == 0xFFFF) // garbage
                        continue;
                    if (l_name->name_2[id] == 0x0000) { // end of name
                        long_name[idx] = '\0';
                        break;
                    }

                    long_name[idx] = static_cast<char>(l_name->name_2[id]);
                    idx++;
                }

                for (int id = 0; id < 2; id++) {
                    if (l_name->name_3[id] == 0xFFFF) // garbage
                        continue;
                    if (l_name->name_3[id] == 0x0000) { // end of name
                        long_name[idx] = '\0';
                        break;
                    }

                    long_name[idx] = static_cast<char>(l_name->name_3[id]);
                    idx++;
                }
                continue;
            }

            if (entry[i].attributes & 0x10) // Dir
                continue;
            if (entry[i].attributes & 0x20) { // File
                if (std::strcmp(long_name, path) == 0) {
                    files.push_back(FAT32File(this, entry[i]));
                    heap::free(buffer);

                    return &files.back();
                }
            }

            if (long_name[0] != '\0')
                long_name[0] = '\0';
        }
        heap::free(buffer);

        return nullptr;
    }

    FAT32File::FAT32File(FAT32FileSystem *filesystem, const FAT32::file_entry &file) : filesystem(filesystem), file(file) {
        const uint32_t first_cluster = static_cast<uint32_t>(file.first_cluster_low) | (static_cast<uint32_t>(file.first_cluster_high) << 16);
        start_lba = filesystem->calculate_lba(first_cluster);
    }

    int64_t FAT32File::read(void *buffer, const size_t size) {
        if (!filesystem || !filesystem->get_dev() || !buffer)
            return -1;

        if (size == 0 || file.file_size == 0)
            return 0;

        if (start_lba == 0)
            return -1;

        const uint64_t bytes_to_read = size < file.file_size ? size : file.file_size;
        if (bytes_to_read > filesystem->get_cluster_size())
            return -1;

        if (!filesystem->get_dev()->read_bytes(start_lba, bytes_to_read, static_cast<uint16_t*>(buffer))) {
            log::error("[ FAT32 ] Read failed: lba=%l size=%u",start_lba, bytes_to_read);
            return -1;
        }

        return static_cast<int64_t>(bytes_to_read);
    }

    int FAT32File::stat(FileStat *info) const {
        info->size = file.file_size;
        info->type = static_cast<u64>(FileType::Regular);
        info->attributes = file.attributes;
        info->created_time = file.creation_time;
        info->modified_time = file.last_modification_time;
        return 0;
    }
}
