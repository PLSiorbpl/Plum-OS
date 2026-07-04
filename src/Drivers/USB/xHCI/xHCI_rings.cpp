#include "xHCI_rings.hpp"

#include "xHCI_common.hpp"
#include "xHCI_mem.hpp"
#include "kernel/log.h"
#include "std/types.hpp"
#include "std/printf.hpp"

namespace USB {
    xhci_command_ring::xhci_command_ring(size_t max_trbs) {
        m_max_trb_count = max_trbs;
        m_rcs_bit = 1;
        m_enqueue_ptr = 0;

        const uint64_t ring_size = max_trbs * sizeof(xhci_trb_t);
        m_trbs = static_cast<xhci_trb_t *>(alloc_xhci_memory(ring_size, XHCI_COMMAND_RING_SEGMENTS_ALIGNMENT, XHCI_COMMAND_RING_SEGMENTS_BOUNDARY));

        m_physical_base = xhci_get_physical_addr(m_trbs);

        m_trbs[max_trbs - 1].parameter = m_physical_base;
        m_trbs[max_trbs - 1].control = (XHCI_TRB_TYPE_LINK << XHCI_TRB_TYPE_SHIFT) | XHCI_LINK_TRB_TC_BIT | m_rcs_bit;
    }

    void xhci_command_ring::enqueue(xhci_trb_t *trb) {
        trb->cycle_bit = m_rcs_bit;
        m_trbs[m_enqueue_ptr] = *trb;

        if (++m_enqueue_ptr == m_max_trb_count - 1) {
            m_trbs[m_max_trb_count - 1].control = (XHCI_TRB_TYPE_LINK << XHCI_TRB_TYPE_SHIFT) | XHCI_LINK_TRB_TC_BIT | m_rcs_bit;
            m_enqueue_ptr = 0;
            m_rcs_bit = !m_rcs_bit;
        }
    }

    xhci_event_ring::xhci_event_ring(const size_t max_trbs, volatile xhci_interrupter_registers *interrupter) {
        m_interrupter_regs = interrupter;
        m_segment_trb_count = max_trbs;
        m_rcs_bit = XHCI_CRCR_RING_CYCLE_STATE;
        m_dequeue_ptr = 0;

        constexpr uint64_t segment_count = 1;
        const uint64_t segment_size = max_trbs * sizeof(xhci_trb_t);
        constexpr uint64_t segment_table_size = segment_count * sizeof(xhci_erst_entry);

        m_trbs = static_cast<xhci_trb_t *>(alloc_xhci_memory(segment_size,
            XHCI_EVENT_RING_SEGMENTS_ALIGNMENT,
            XHCI_EVENT_RING_SEGMENTS_BOUNDARY)
            );

        m_physical_base = xhci_get_physical_addr(m_trbs);

        m_segment_table = static_cast<xhci_erst_entry *>(alloc_xhci_memory(segment_table_size,
            XHCI_EVENT_RING_SEGMENT_TABLE_ALIGNMENT,
            XHCI_EVENT_RING_SEGMENT_TABLE_BOUNDARY)
            );

        xhci_erst_entry entry{};
        entry.ring_segment_base_address = m_physical_base;
        entry.ring_segment_size = m_segment_trb_count;
        entry.rsvd = 0;

        m_segment_table[0] = entry;

        m_interrupter_regs->erstsz = 1;
        _update_erdp();
        m_interrupter_regs->erstba = xhci_get_physical_addr(m_segment_table);
    }

    bool xhci_event_ring::has_unprocessed_events() const {
        return (m_trbs[m_dequeue_ptr].cycle_bit == m_rcs_bit);
    }

    void xhci_event_ring::dequeue_events(std::vector<xhci_trb_t *> &trbs) {
        while (has_unprocessed_events()) {
            xhci_trb_t *trb = _dequeue_trb();
            if (!trb) {
                break;
            }

            trbs.push_back(trb);
        }

        _update_erdp();

        uint64_t erdp = m_interrupter_regs->erdp;
        erdp |= XHCI_ERDP_EHB;
        m_interrupter_regs->erdp = erdp;
    }

    void xhci_event_ring::flush_unprocessed_events() {
        std::vector<xhci_trb_t *> events;
        dequeue_events(events);
        events.clear();
    }

    void xhci_event_ring::_update_erdp() const {
        const uint64_t dequeue_address = m_physical_base + (m_dequeue_ptr * sizeof(xhci_trb_t));
        m_interrupter_regs->erdp = dequeue_address;
    }

    xhci_trb_t *xhci_event_ring::_dequeue_trb() {
        if (m_trbs[m_dequeue_ptr].cycle_bit != m_rcs_bit) {
            log::error("Event Ring attempted to dequeue an invalid TRB, returning nullptr!\n");
            return nullptr;
        }

        xhci_trb_t* ret = &m_trbs[m_dequeue_ptr];

        if (++m_dequeue_ptr == m_segment_trb_count) {
            m_dequeue_ptr = 0;
            m_rcs_bit = !m_rcs_bit;
        }

        return ret;
    }

    int32_t xhci_transfer_ring::init(size_t max_trbs, uint8_t doorbell_id) {
    m_max_trb_count = max_trbs;
    m_rcs_bit = 1;
    m_dequeue_ptr = 0;
    m_enqueue_ptr = 0;
    m_doorbell_id = doorbell_id;

    const size_t ring_size = max_trbs * sizeof(xhci_trb_t);

    m_trbs = static_cast<xhci_trb_t*>(
        alloc_xhci_memory(ring_size));
    if (!m_trbs) {
        log::error("xhci: failed to allocate transfer ring (%lu TRBs)", max_trbs);
        return -1;
    }

    m_physical_base = xhci_get_physical_addr(m_trbs);

    // Set the last TRB as a link TRB to point back to the first TRB
    m_trbs[m_max_trb_count - 1].parameter = m_physical_base;
    m_trbs[m_max_trb_count - 1].control =
        (XHCI_TRB_TYPE_LINK << XHCI_TRB_TYPE_SHIFT) | XHCI_LINK_TRB_TC_BIT | m_rcs_bit;

    return 0;
}

void xhci_transfer_ring::destroy() {
    if (m_trbs) {
        free_xhci_memory(m_trbs);
        m_trbs = nullptr;
    }
}

uintptr_t xhci_transfer_ring::get_enqueue_phys() const {
    return m_physical_base + m_enqueue_ptr * sizeof(xhci_trb_t);
}

bool xhci_transfer_ring::can_enqueue(size_t n) const {
    // Usable slots = m_max_trb_count - 1 (last slot is the Link TRB)
    size_t usable = m_max_trb_count - 1;
    size_t available = (m_enqueue_ptr < usable)
        ? (usable - m_enqueue_ptr)
        : 0;
    return n <= available;
}

void xhci_transfer_ring::enqueue(xhci_trb_t* trb) {
    // Adjust the TRB's cycle bit to the current DCS
    trb->cycle_bit = m_rcs_bit;

    // Insert the TRB into the ring
    m_trbs[m_enqueue_ptr] = *trb;

    // Advance and possibly wrap the enqueue pointer if needed.
    // maxTrbCount - 1 accounts for the LINK_TRB.
    if (++m_enqueue_ptr == m_max_trb_count - 1) {
        // Only now update the Link TRB, syncing its cycle bit and setting the TC flag.
        m_trbs[m_max_trb_count - 1].control =
            (XHCI_TRB_TYPE_LINK << XHCI_TRB_TYPE_SHIFT) | XHCI_LINK_TRB_TC_BIT | m_rcs_bit;

        m_enqueue_ptr = 0;
        m_rcs_bit = !m_rcs_bit;
    }
}
}
