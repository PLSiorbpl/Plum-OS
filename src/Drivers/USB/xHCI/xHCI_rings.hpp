#pragma once
#include "xHCI_trb.hpp"
#include "std/types.hpp"
#include "xHCI_regs.hpp"
#include "std/vector.hpp"

namespace USB {
    class xhci_command_ring {
    public:
        xhci_command_ring(size_t max_trbs);

        [[nodiscard]] xhci_trb_t* get_virtual_base() const { return m_trbs; }
        [[nodiscard]] uintptr_t get_physical_base() const { return m_physical_base; }
        [[nodiscard]] uint8_t  get_cycle_bit() const { return m_rcs_bit; }

        void enqueue(xhci_trb_t *trb);

    private:
        size_t m_max_trb_count;
        size_t m_enqueue_ptr;
        xhci_trb_t* m_trbs;
        uintptr_t m_physical_base;
        uint8_t m_rcs_bit;
    };

    struct xhci_erst_entry {
        uint64_t ring_segment_base_address;
        uint32_t ring_segment_size;
        uint32_t rsvd;
    } __attribute__((packed));

    class xhci_event_ring {
    public:
        xhci_event_ring(size_t max_trbs, volatile xhci_interrupter_registers* interrupter);

        [[nodiscard]] xhci_trb_t* get_virtual_base() const { return m_trbs; }
        [[nodiscard]] uintptr_t get_physical_base() const { return m_physical_base; }
        [[nodiscard]] uint8_t  get_cycle_bit() const { return m_rcs_bit; }

        bool has_unprocessed_events() const;
        void dequeue_events(std::vector<xhci_trb_t*>& trbs);

        void flush_unprocessed_events();

    private:
        volatile xhci_interrupter_registers* m_interrupter_regs;

        size_t m_segment_trb_count;

        xhci_trb_t* m_trbs;
        uintptr_t m_physical_base;

        xhci_erst_entry* m_segment_table;

        uint64_t m_dequeue_ptr;
        uint8_t m_rcs_bit;

    private:
        void _update_erdp() const;
        xhci_trb_t* _dequeue_trb();
    };

    class xhci_transfer_ring {
    public:
        xhci_transfer_ring() = default;
        int32_t init(size_t max_trbs, uint8_t doorbell_id);
        void destroy();

        inline xhci_trb_t* get_virtual_base() const { return m_trbs; }
        inline uintptr_t get_physical_base() const { return m_physical_base; }
        inline uint8_t  get_cycle_bit() const { return m_rcs_bit; }
        inline uint8_t get_doorbell_id() const { return m_doorbell_id; }
        inline size_t  get_max_trb_count() const { return m_max_trb_count; }

        uintptr_t get_enqueue_phys() const;
        bool can_enqueue(size_t n) const;

        void enqueue(xhci_trb_t* trb);

    private:
        size_t              m_max_trb_count = 0; // Number of valid TRBs in the ring including the LINK_TRB
        size_t              m_dequeue_ptr = 0;   // Transfer ring consumer dequeue pointer
        size_t              m_enqueue_ptr = 0;   // Transfer ring producer enqueue pointer
        xhci_trb_t*         m_trbs = nullptr;    // Base address of the ring buffer
        uintptr_t           m_physical_base = 0;
        uint8_t             m_rcs_bit = 0;       // Dequeue cycle state
        uint8_t             m_doorbell_id = 0;   // ID of the doorbell associated with the ring
    };
}
