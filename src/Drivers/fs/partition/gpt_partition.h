#pragma once
#include "std/types.hpp"

namespace fs::partition {
    struct gpt_header {
        u64 signature;
        u32 revision;
        u32 header_size;
        u32 header_checksum;
        u32 reserved;
        u64 lba_header;
        u64 lba_alt_header;
        u64 first_usable_lba;
        u64 last_usable_lba;
        u8 disk_guid[16];
        u64 partition_entry_lba;
        u32 partition_entry_count;
        u32 partition_entry_size;
        u32 partition_entry_checksum;
    } __attribute__((packed));

    struct gpt_partition {
        u8 type_guid[16];
        u8 unique_guid[16];
        u64 starting_lba;
        u64 ending_lba;
        u64 attributes;
        u16 name[36];
    } __attribute__((packed));
}
