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
        data_sectors = fat.bpb.total_sectors - fat.bpb.rsvd_sector_count - (fat.bpb.num_of_FATs * fat.bpb.ebpb.sectors_per_FAT);
        max_cluster = (data_sectors/fat.bpb.sectors_per_cluster) + 1;

        return true;
    }

    FileObject *FAT32FileSystem::open(const char *path, int flags) {
        if (path[0] == '\0') {
            FAT32::file_entry entry = {};
            entry.file_name[0] = '\0';
            entry.attributes = 0x10;
            entry.file_size = 0;
            entry.first_cluster_high = (root_cluster >> 16) & 0xFFFF;
            entry.first_cluster_low = root_cluster & 0xFFFF;
            auto ptr = std::make_unique<FAT32Dir>(this, entry, 0, 0);
            files.push_back(std::move(ptr));

            return files.back().get();
        }

        const char *i = path;
        const char *last_i = i;
        uint64_t lba = root_lba;
        while (true) {
            auto file = find_entry(&lba, i);
            last_i = i;
            i = std::strchr(i, '/');

            if (!file) {
                if (last_i && i == nullptr && (flags & O_CREAT)) {
                    file = create_entry(&lba, last_i);
                    if (!file) return nullptr;
                    auto f = std::unique_ptr(file);
                    files.push_back(std::move(f));
                    return files.back().get();
                }
                return nullptr;
            }

            if (i == nullptr) {
                auto f = std::unique_ptr(file);
                files.push_back(std::move(f));
                return files.back().get();
            }
            i++;
            if (!file->is_directory())
                return nullptr;
            delete file;
        }
        return nullptr;
    }

    FileObject *FAT32FileSystem::create_entry(uint64_t *lba, const char *name) {
        auto *buffer = static_cast<FAT32::file_entry *>(heap::malloc(cluster_size));
        if (!buffer)
            return nullptr;

        if (!device->read_bytes(*lba, 0, cluster_size, reinterpret_cast<uint16_t *>(buffer))) {
            heap::free(buffer);
            log::error("[ FAT32 ] Failed to read cluster: lba=%l cluster_size=%u", *lba, cluster_size);
            return nullptr;
        }

        for (size_t i = 0; i < cluster_size/sizeof(FAT32::file_entry); i++) {
            if (buffer[i].file_name[0] == 0x0 || buffer[i].file_name[0] == 0xE5) { // Nothing || Deleted file
                FAT32::file_entry entry = {};
                entry.attributes = 0x20; // File
                entry.file_size = 0;
                entry.first_cluster_low = 0;
                entry.first_cluster_high = 0;
                if (!make_short_name(name, &entry)) {
                    heap::free(buffer);
                    return nullptr;
                }

                if (!device->write_bytes(*lba, i * sizeof(FAT32::file_entry), sizeof(FAT32::file_entry), reinterpret_cast<uint16_t *>(&entry))) {
                    heap::free(buffer);
                    log::error("[ FAT32 ] Failed to write entry: lba=%l cluster_size=%u", *lba, cluster_size);
                    return nullptr;
                }
                FileObject *ptr = new FAT32File(this, entry, i * sizeof(FAT32::file_entry), *lba);
                heap::free(buffer);
                return ptr;
            }
        }
        heap::free(buffer);
        return nullptr;
    }

    FileObject *FAT32FileSystem::find_entry(uint64_t *lba, const char *entry) {
        auto *buffer = static_cast<FAT32::file_entry *>(heap::malloc(cluster_size));
        if (!buffer)
            return nullptr;

        if (!device->read_bytes(*lba, 0, cluster_size, reinterpret_cast<uint16_t *>(buffer))) {
            heap::free(buffer);
            log::error("[ FAT32 ] Failed to read cluster: lba=%l cluster_size=%u", *lba, cluster_size);
            return nullptr;
        }

        char long_name[255] = {};
        for (size_t i = 0; i < cluster_size/sizeof(FAT32::file_entry); i++) {
            if (buffer[i].file_name[0] == 0x0) // Nothing
                break;
            if (buffer[i].file_name[0] == 0xE5) { // Deleted file
                long_name[0] = '\0';
                continue;
            }

            char short_name[13] = {};
            parse_short_name(short_name, &buffer[i]);

            const char* name = long_name[0] ? long_name : short_name;

            // Long file name
            if (buffer[i].attributes == 0x0F) {
                parse_lfn(long_name, reinterpret_cast<FAT32::long_file_name *>(&buffer[i]));
                name = long_name;
                continue;
            }

            if (buffer[i].attributes & 0x08)
                continue;

            const char* slash = std::strchr(entry, '/');
            size_t len = slash ? static_cast<size_t>(slash - entry) : std::strlen(entry);

            if (std::strncmp(name, entry, len) == 0 && name[len] == '\0') {
                FileObject *ptr = nullptr;
                if (buffer[i].attributes & 0x10)
                    ptr = new FAT32Dir(this, buffer[i], i * sizeof(FAT32::file_entry), *lba);
                else
                    ptr = new FAT32File(this, buffer[i], i * sizeof(FAT32::file_entry), *lba);

                uint64_t cluster = static_cast<uint32_t>(buffer[i].first_cluster_low) | (static_cast<uint32_t>(buffer[i].first_cluster_high) << 16);
                if (cluster == 0)
                    cluster = root_cluster;
                *lba = calculate_lba(cluster);
                heap::free(buffer);
                return ptr;
            }

            if (long_name[0] != '\0')
                long_name[0] = '\0';
        }

        heap::free(buffer);
        return nullptr;
    }

    bool FAT32FileSystem::make_short_name(const char *name, FAT32::file_entry *entry) {
        if (!name || !entry) return false;

        memset(entry->file_name, ' ', 11);
        entry->rsvd_WindowsNT = 0;

        const char *dot = std::strrchr(name, '.');
        size_t base_len = dot ? dot - name : std::strlen(name);
        size_t ext_len = dot ? std::strlen(dot + 1) : 0;

        if (base_len == 0 || base_len > 8 || ext_len > 3)
            return false;

        bool base_lower = false;
        bool ext_lower = false;

        for (int i = 0; i < base_len; i++) {
            char c = name[i];

            if (c == ' ' || c == '.' || c == '/' || c == '\\')
                return false;

            if (std::islower(c)) {
                base_lower = true;
                c = static_cast<char>(std::toupper(c));
            }
            entry->file_name[i] = c;
        }

        for (int i = 0; i < ext_len; i++) {
            char c = dot[1 + i];

            if (c == ' ' || c == '.' || c == '/' || c == '\\')
                return false;

            if (std::islower(c)) {
                ext_lower = true;
                c = static_cast<char>(std::toupper(c));
            }
            entry->file_name[8 + i] = c;
        }

        if (base_lower) entry->rsvd_WindowsNT |= 0x8;
        if (ext_lower) entry->rsvd_WindowsNT |= 0x10;
        return true;
    }

    void FAT32FileSystem::parse_short_name(char *buf, const FAT32::file_entry *entry) {
        size_t pos = 0;
        for (int n = 0; n < 8 && entry->file_name[n] != ' '; n++) {
            if (entry->rsvd_WindowsNT & 0x8)
                buf[pos++] = static_cast<char>(std::tolower(entry->file_name[n]));
            else
                buf[pos++] = entry->file_name[n];
        }

        int ext_len = 3;
        while (ext_len > 0 && entry->file_name[8 + ext_len - 1] == ' ')
            ext_len--;

        if (ext_len > 0) {
            buf[pos++] = '.';

            for (int n = 0; n < ext_len; n++) {
                if (entry->rsvd_WindowsNT & 0x10)
                    buf[pos++] = static_cast<char>(std::tolower(entry->file_name[8 + n]));
                else
                    buf[pos++] = entry->file_name[8 + n];
            }
        }

        buf[pos] = '\0';
    }

    /**
     * @brief Parses long_file_name
     * @param buf char buffer for long_file_name (max char buf[255])
     * @param entry long_file_name entry
     */
    void FAT32FileSystem::parse_lfn(char *buf, const FAT32::long_file_name *entry) {
        uint8_t idx = ((entry->order & 0x1F) - 1) * 13;
        if (idx >= 255) {
            buf[254] = '\0';
            return;
        }

        for (int id = 0; id < 5; id++) {
            if (entry->name_1[id] == 0xFFFF) // garbage
                break;
            if (entry->name_1[id] == 0x0000) { // end of name
                buf[idx] = '\0';
                break;
            }

            buf[idx] = static_cast<char>(entry->name_1[id]);
            idx++;
        }

        for (int id = 0; id < 6; id++) {
            if (entry->name_2[id] == 0xFFFF) // garbage
                break;
            if (entry->name_2[id] == 0x0000) { // end of name
                buf[idx] = '\0';
                break;
            }

            buf[idx] = static_cast<char>(entry->name_2[id]);
            idx++;
        }

        for (int id = 0; id < 2; id++) {
            if (entry->name_3[id] == 0xFFFF) // garbage
                break;
            if (entry->name_3[id] == 0x0000) { // end of name
                buf[idx] = '\0';
                break;
            }

            buf[idx] = static_cast<char>(entry->name_3[id]);
            idx++;
        }
    }

    bool FAT32FileSystem::get_next_cluster(const uint64_t cluster, uint64_t *next) const {
        if (!next || !device || fat.bpb.bytes_per_sector == 0 || cluster < 2 || cluster > max_cluster)
            return false;

        const bool mirroring_disabled = fat.bpb.ebpb.flags & 0x80;
        const uint64_t active_fat = mirroring_disabled ? (fat.bpb.ebpb.flags & 0x0F) : 0;
        if (active_fat >= fat.bpb.num_of_FATs)
            return false;

        const uint64_t fat_start_lba = fat.gpt_partition.starting_lba + fat.bpb.rsvd_sector_count + active_fat * fat.bpb.ebpb.sectors_per_FAT;
        const uint64_t fat_offset = cluster * 4;
        const uint64_t fat_sector = fat_start_lba + fat_offset / fat.bpb.bytes_per_sector;
        const uint32_t entry_offset = fat_offset % fat.bpb.bytes_per_sector;

        uint32_t value;
        if (!device->read_bytes(fat_sector, entry_offset, sizeof(uint32_t), reinterpret_cast<uint16_t *>(&value))) {
            log::error("[ FAT32 ] Read failed: sector=%l offset=%l size=%uB", fat_sector, entry_offset, sizeof(uint32_t));
            return false;
        }

        value &= 0x0FFFFFFF;

        if (value >= 0x0FFFFFF8) {
            *next = 0;
            return true;
        }

        if (value < 2 || value >= 0x0FFFFFF0 || value > max_cluster)
            return false;

        *next = value;
        return true;
    }

    bool FAT32FileSystem::find_free_cluster(uint64_t *cluster) const {
        if (!cluster || !device || fat.bpb.bytes_per_sector == 0)
            return false;

        const bool mirroring_disabled = fat.bpb.ebpb.flags & 0x80;
        const uint64_t active_fat = mirroring_disabled ? (fat.bpb.ebpb.flags & 0x0F) : 0;
        if (active_fat >= fat.bpb.num_of_FATs)
            return false;

        const uint64_t fat_start_lba = fat.gpt_partition.starting_lba + fat.bpb.rsvd_sector_count + active_fat * fat.bpb.ebpb.sectors_per_FAT;

        const auto sector = static_cast<uint16_t *>(heap::malloc(fat.bpb.bytes_per_sector));
        if (!sector)
            return false;

        for (uint64_t sector_index = 0; sector_index < fat.bpb.ebpb.sectors_per_FAT; sector_index++) {
            if (!device->read(fat_start_lba + sector_index, 1, sector)) {
                heap::free(sector);
                log::error("[ FAT32 ] Read failed: sector=%l size=%uB", fat_start_lba + sector_index, fat.bpb.bytes_per_sector);
                return false;
            }

            uint32_t value;
            for (int i = 0; i < fat.bpb.bytes_per_sector / sizeof(uint32_t); i++) {
                const uint32_t cluster_index = sector_index * (fat.bpb.bytes_per_sector / sizeof(uint32_t)) + i;
                if (cluster_index < 2)
                    continue;
                if (cluster_index >= max_cluster) {
                    heap::free(sector);
                    return false;
                }
                memcpy(&value, reinterpret_cast<uint8_t*>(sector) + i * sizeof(uint32_t), sizeof(uint32_t));
                value &= 0x0FFFFFFF;
                if (value == 0x0) {
                    *cluster = cluster_index;
                    heap::free(sector);
                    return true;
                }
            }
        }

        heap::free(sector);
        return false;
    }

    /**
     * @brief Set cluster to value (EOF, next_cluster)
     * @param cluster
     * @param value
     * @return success - true | failed - false
     */
    bool FAT32FileSystem::set_cluster(const uint64_t cluster, uint32_t value) const {
        if (!device || fat.bpb.bytes_per_sector == 0 || fat.bpb.num_of_FATs == 0 || fat.bpb.ebpb.sectors_per_FAT == 0 || cluster < 2 || cluster > max_cluster)
            return false;

        const bool mirroring_disabled = fat.bpb.ebpb.flags & 0x80;
        const uint64_t active_fat = mirroring_disabled ? (fat.bpb.ebpb.flags & 0x0F) : 0;
        if (active_fat >= fat.bpb.num_of_FATs)
            return false;

        uint64_t fat_start_lba = fat.gpt_partition.starting_lba + fat.bpb.rsvd_sector_count + active_fat * fat.bpb.ebpb.sectors_per_FAT;
        const uint64_t fat_offset = cluster * 4;
        uint64_t fat_sector = fat_start_lba + fat_offset / fat.bpb.bytes_per_sector;
        const uint32_t entry_offset = fat_offset % fat.bpb.bytes_per_sector;

        value &= 0x0FFFFFFF;

        uint32_t old;
        if (!device->read_bytes(fat_sector, entry_offset, sizeof(uint32_t), reinterpret_cast<uint16_t *>(&old))) {
            log::error("[ FAT32 ] Read failed: sector=%l offset=%l size=%uB", fat_sector, entry_offset, sizeof(uint32_t));
            return false;
        }

        old &= 0xF0000000;
        value |= old;

        if (!device->write_bytes(fat_sector, entry_offset, sizeof(uint32_t), reinterpret_cast<uint16_t *>(&value))) {
            log::error("[ FAT32 ] Write failed: sector=%l offset=%l size=%uB", fat_sector, entry_offset, sizeof(uint32_t));
            return false;
        }

        // Write to backups
        bool succes = true;
        if (!mirroring_disabled) {
            for (uint32_t fat_index = 1; fat_index < fat.bpb.num_of_FATs; fat_index++) {
                fat_start_lba = fat.gpt_partition.starting_lba + fat.bpb.rsvd_sector_count + fat_index * fat.bpb.ebpb.sectors_per_FAT;
                fat_sector = fat_start_lba + fat_offset / fat.bpb.bytes_per_sector;

                if (!device->write_bytes(fat_sector, entry_offset, sizeof(uint32_t), reinterpret_cast<uint16_t*>(&value))) {
                    log::error("[ FAT32 ] Write to backup failed: sector=%l offset=%l size=%uB", fat_sector, entry_offset, sizeof(uint32_t));
                    succes = false;
                }
            }
        }

        return succes;
    }

    void FAT32FileSystem::close(const FileObject *file) {
        if (!file) return;
        for (int i = 0; i < files.size(); i++) {
            if (files[i].get() == file) {
                files.erase(i);
                return;
            }
        }
    }
}
