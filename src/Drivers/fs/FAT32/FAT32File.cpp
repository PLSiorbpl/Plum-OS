#include "FAT32FileSystem.hpp"
#include "Data_structures.hpp"
#include "std/types.hpp"
#include "kernel/Process/FileDescriptor.hpp"

using namespace vfs;

namespace fs {
    FAT32File::FAT32File(FAT32FileSystem *filesystem, const FAT32::file_entry &file, const uint64_t directory_entry_offset, const uint64_t directory_entry_lba)
    : filesystem(filesystem), file(file), directory_entry_offset(directory_entry_offset), directory_entry_lba(directory_entry_lba) {
        first_cluster = static_cast<uint32_t>(file.first_cluster_low) | (static_cast<uint32_t>(file.first_cluster_high) << 16);
        start_lba = filesystem->calculate_lba(first_cluster);
        cluster_size = filesystem->get_cluster_size();
    }

    bool FAT32File::update_file_entry() {
        if (!filesystem || !filesystem->get_dev())
            return false;

        if (!filesystem->get_dev()->write_bytes(directory_entry_lba, directory_entry_offset, sizeof(FAT32::file_entry), reinterpret_cast<uint16_t *>(&file))) {
            return false;
        }
        return true;
    }

    int64_t FAT32File::write(const void *buffer, const size_t size) {
        if (!filesystem || !filesystem->get_dev() || !buffer)
            return -1;

        if (size == 0)
            return 0;

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

        uint64_t written = 0;
        uint64_t offset_in_cluster = position % cluster_size;

        while (written < size) {
            const uint64_t available = cluster_size - offset_in_cluster;
            const uint64_t chunk = size - written < available ? size - written : available;
            const uint64_t cluster_lba = filesystem->calculate_lba(cluster);

            if (!filesystem->get_dev()->write_bytes(cluster_lba, offset_in_cluster, chunk, reinterpret_cast<const uint16_t *>(static_cast<const uint8_t *>(buffer) + written))) {
                position += written;
                return written > 0 ? static_cast<int64_t>(written) : -1;
            }

            written += chunk;
            offset_in_cluster = 0;

            if (written < size) {
                uint64_t next;

                if (!filesystem->get_next_cluster(cluster, &next)) {
                    position += written;
                    return written > 0 ? static_cast<int64_t>(written) : -1;
                }

                if (next == 0)
                    return -1;

                cluster = next;
            }
        }

        position += written;
        if (position > file.file_size) {
            file.file_size = position;
            update_file_entry();
        }
        return static_cast<int64_t>(size);
    }

    int64_t FAT32File::read(void *buffer, const size_t size) {
        if (!filesystem || !filesystem->get_dev() || !buffer || cluster_size == 0 || start_lba < 2)
            return -1;

        if (size == 0 || position >= file.file_size)
            return 0;

        const uint64_t remaining_file = file.file_size - position;
        const uint64_t bytes_to_read = size < remaining_file ? size : remaining_file;

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

            if (!filesystem->get_dev()->read_bytes(cluster_lba, 0, static_cast<int32_t>(cluster_size), reinterpret_cast<uint16_t*>(temp))) {
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

    int FAT32File::truncate(const uint64_t lenght) {
        if (!filesystem || !filesystem->get_dev())
            return -1;

        if (lenght > cluster_size)
            return -1;

        auto old_size = file.file_size;
        file.file_size = lenght;
        if (!update_file_entry()) {
            file.file_size = old_size;
            return -1;
        }
        return 0;
    }

    void FAT32File::close() {
        if (!filesystem)
            return;
        filesystem->close(this);
    }
}
