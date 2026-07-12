#include "xHCI_mem.hpp"

#include <std/mem_common.hpp>

#include "kernel/log.h"
#include "kernel/Memory/heap.hpp"
#include "kernel/Paging.hpp"
#include "kernel/Memory/mem_helper.h"

namespace USB {
    uintptr_t xhci_map_mmio(const uint64_t pci_bar_address, const uint32_t bar_size) {
        Paging::Map_memory(pci_bar_address, pci_bar_address+bar_size, Paging::Profile::MMIO);
        return pci_bar_address;
    }

    void *alloc_xhci_memory(const uint64_t size, const uint64_t alignment, const uint64_t boundary) {
        if (size == 0) {
            log::error("Attempted DMA allocation with size 0!\n");
        }

        if (alignment == 0) {
            log::error("Attempted DMA allocation with alignment 0!\n");
        }

        if (boundary == 0) {
            log::error("Attempted DMA allocation with boundary 0!\n");
        }

        void* memblock = heap::malloc_boundry(size, alignment, boundary);

        if (!memblock) {
            log::error("======= MEMORY ALLOCATION FAILED =======\n");
        }

        std::memset(memblock, 0, size);
        return memblock;
    }

    void free_xhci_memory(void *ptr) {
        // TODO
        // never frfr
    }

    uintptr_t xhci_get_physical_addr(const void *vaddr) {
        const auto paddr = to_physical(vaddr);
        return paddr;
    }
}
