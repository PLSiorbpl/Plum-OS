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

    bool FAT32FileSystem::get_next_cluster(uint64_t cluster, uint64_t *next) {
        if (!next)
            return false;

        const uint64_t fat_start_lba = fat.gpt_partition.starting_lba + fat.bpb.rsvd_sector_count;
        const uint64_t fat_offset = cluster * 4;
        const uint64_t fat_sector = fat_start_lba + fat_offset / fat.bpb.bytes_per_sector;
        const uint32_t entry_offset = fat_offset % fat.bpb.bytes_per_sector;

        const auto sector = static_cast<uint16_t *>(heap::malloc(fat.bpb.bytes_per_sector));
        if (!device->read(fat_sector, 1, sector)) {
            log::error("[ FAT32 ] Read failed: sector=%l size=%uB", fat_sector, 512);
            return false;
        }

        uint32_t value;
        memcpy(&value, reinterpret_cast<uint8_t*>(sector) + entry_offset, sizeof(value));
        heap::free(sector);

        value &= 0x0FFFFFFF;

        if (value >= 0x0FFFFFF8) {
            *next = 0;
            return true;
        }

        if (value < 2 || value >= 0x0FFFFFF0)
            return false;

        *next = value;
        return true;
    }

    FAT32File::FAT32File(FAT32FileSystem *filesystem, const FAT32::file_entry &file) : filesystem(filesystem), file(file) {
        first_cluster = static_cast<uint32_t>(file.first_cluster_low) | (static_cast<uint32_t>(file.first_cluster_high) << 16);
        start_lba = filesystem->calculate_lba(first_cluster);
    }

    int64_t FAT32File::read(void *buffer, const size_t size) {
        if (!filesystem || !filesystem->get_dev() || !buffer)
            return -1;

        if (size == 0 || position >= file.file_size)
            return 0;

        const uint64_t remaining_file = file.file_size - position;
        const uint64_t bytes_to_read = size < remaining_file ? size : remaining_file;
        const uint64_t cluster_size = filesystem->get_cluster_size();
        if (cluster_size == 0 || start_lba < 2)
            return -1;

        uint64_t cluster = first_cluster;
        const uint64_t cluster_index = position / cluster_size;

        for (uint64_t i = 0; i < cluster_index; ++i) {
            uint64_t next;

            if (!filesystem->get_next_cluster(cluster, &next))
                return -1;

            if (next == 0)
                return -1;

            cluster = next;
        }

        auto* temp = static_cast<uint8_t*>(heap::malloc(cluster_size));
        if (!temp)
            return -1;

        uint64_t copied = 0;
        uint64_t offset_in_cluster = position % cluster_size;

        while (copied < bytes_to_read) {
            const uint64_t available = cluster_size - offset_in_cluster;
            const uint64_t chunk = (bytes_to_read - copied < available) ? bytes_to_read - copied : available;
            const uint64_t cluster_lba = filesystem->calculate_lba(cluster);

            if (!filesystem->get_dev()->read_bytes(cluster_lba, static_cast<int32_t>(cluster_size), reinterpret_cast<uint16_t*>(temp))) {
                heap::free(temp);
                position += copied;
                return copied > 0 ? static_cast<int64_t>(copied) : -1;
            }

            memmove(static_cast<uint8_t*>(buffer) + copied, temp + offset_in_cluster, chunk);
            copied += chunk;
            offset_in_cluster = 0;

            if (copied < bytes_to_read) {
                uint64_t next;

                if (!filesystem->get_next_cluster(cluster, &next)) {
                    heap::free(temp);
                    position += copied;
                    return copied > 0 ? static_cast<int64_t>(copied) : -1;
                }

                if (next == 0)
                    return -1;

                cluster = next;
            }
        }

        heap::free(temp);
        position += copied;

        return static_cast<int64_t>(copied);
    }

    int FAT32File::stat(FileStat *info) const {
        info->size = file.file_size;
        info->type = static_cast<u64>(FileType::Regular);
        info->attributes = file.attributes;
        info->created_time = file.creation_time;
        info->modified_time = file.last_modification_time;
        return 0;
    }

    int64_t FAT32File::seek(const int64_t offset, unsigned int whence) {
        if (!filesystem)
            return -1;

        int64_t base = 0;

        switch (static_cast<SeekMode>(whence)) {
            case SeekMode::SEEK_SET:
                base = 0;
                break;

            case SeekMode::SEEK_CUR:
                if (position > INT64_MAX)
                    return -1;

                base = static_cast<int64_t>(position);
                break;

            case SeekMode::SEEK_END:
                if (file.file_size > INT64_MAX)
                    return -1;

                base = static_cast<int64_t>(file.file_size);
                break;

            default:
                return -1;
        }

        if (offset > 0 && base > INT64_MAX - offset)
            return -1;
        if (offset < 0 && base < INT64_MIN - offset)
            return -1;
        const int64_t new_position = base + offset;

        if (new_position < 0 ||
        static_cast<uint64_t>(new_position) > file.file_size)
            return -1;

        position = static_cast<uint64_t>(new_position);

        return new_position;
    }
}
