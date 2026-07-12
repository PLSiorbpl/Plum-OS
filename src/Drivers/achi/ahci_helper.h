#pragma once
#include "ahci_port.h"
#include "kernel/Memory/heap.hpp"
#include "std/mem_common.hpp"
#include "std/types.hpp"
#include "std/printf.hpp"

namespace drivers::ahci {
    inline void* allocate_virtual_memory(u64 size, u64 alignment) {
        const auto out = heap::malloc_align(size, alignment);
        //Paging::Map_memory(reinterpret_cast<u64>(out), reinterpret_cast<u64>(out) + size, Paging::Profile::MMIO);
        std::memset(out, 0, size);
        return out;
    }

    inline port_type get_port_type(const volatile hba_port* port) {
        const u32 sata_status = port->ssts;

        const u8 interface_power_management = (sata_status >> 8) & 0x0F;
        const u8 device_detection = sata_status & 0x7;

        if (device_detection != 0x03 || interface_power_management != 0x1)
            return port_type::none;

        switch (port->sig) {
            case SATA_SIG_ATAPI:
                return port_type::satapi;
            case SATA_SIG_ATA:
                return port_type::sata;
            case SATA_SIG_SEMB:
                return port_type::semb;
            case SATA_SIG_PM:
                return port_type::pm;
            default: {
                std::kernel::printf("&4Unknown port type %x", port->sig);
                return port_type::none;
            }
        }
    }
}