#pragma once
#include "std/types.hpp"

constexpr uint8_t ACCESS_PRESENT    = (1 << 7);
constexpr uint8_t ACCESS_RING0      = (0 << 5);
constexpr uint8_t ACCESS_RING3      = (3 << 5);
constexpr uint8_t ACCESS_CODE_SEG   = (1 << 4) | (1 << 3);
constexpr uint8_t ACCESS_DATA_SEG   = (1 << 4);
constexpr uint8_t ACCESS_READABLE   = (1 << 1);
constexpr uint8_t ACCESS_WRITABLE   = (1 << 1);

constexpr uint8_t FLAG_GRANULARITY  = (1 << 3);
constexpr uint8_t FLAG_32BIT        = (1 << 2);
constexpr uint8_t FLAG_64BIT        = (1 << 1);

constexpr uint8_t ACCESS_TSS = 0x89;

struct gdtd {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct tss_entry {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset;
} __attribute__((packed));

void init_tss();

extern gdtd gdt_descriptor;
extern uint64_t gdt[];
extern tss_entry tss;
