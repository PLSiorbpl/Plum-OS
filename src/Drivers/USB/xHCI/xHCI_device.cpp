#include "xHCI_device.hpp"
#include "std/types.hpp"
#include "xHCI_mem.hpp"
#include "std/mem_common.hpp"
#include "kernel/Paging.hpp"
#include "xHCI_rings.hpp"
#include "kernel/log.h"
#include "kernel/Memory/mem_helper.h"

namespace USB {
    xhci_device::xhci_device(const uint8_t port, const uint8_t slot, const uint8_t speed, const bool use_64byte_ctx)
    : m_port(port), m_slot(slot), m_speed(speed), m_use64byte_ctx(use_64byte_ctx) {
        _alloc_input_ctx();
    }

    void xhci_device::_alloc_input_ctx() {
        const uint64_t input_context_size = m_use64byte_ctx ? sizeof(xhci_input_context64) : sizeof(xhci_input_context32);

        m_input_ctx = alloc_xhci_memory(input_context_size, XHCI_INPUT_CONTROL_CONTEXT_ALIGNMENT, XHCI_INPUT_CONTROL_CONTEXT_BOUNDARY);
        m_input_ctx_dma_addr = xhci_get_physical_addr(m_input_ctx);

        m_ctrl_transfer_buffer = alloc_xhci_memory(PAGE_SIZE);
        m_ctrl_transfer_buffer_phys = static_cast<uintptr_t>(to_physical(m_ctrl_transfer_buffer));

        m_ctrl_ring = new xhci_transfer_ring();
        m_ctrl_ring->init(XHCI_TRANSFER_RING_TRB_COUNT, m_slot);
    }

    xhci_input_control_context32* xhci_device::get_input_ctrl_ctx() const {
        if (m_use64byte_ctx) {
            auto* input_ctx = static_cast<xhci_input_context64*>(m_input_ctx);
            return reinterpret_cast<xhci_input_control_context32*>(&input_ctx->control_context);
        }
        auto* input_ctx = static_cast<xhci_input_context32*>(m_input_ctx);
        return &input_ctx->control_context;
    }

    xhci_slot_context32* xhci_device::get_input_slot_ctx() const {
        if (m_use64byte_ctx) {
            auto* input_ctx = static_cast<xhci_input_context64*>(m_input_ctx);
            return reinterpret_cast<xhci_slot_context32*>(&input_ctx->device_context.slot_context);
        }
        auto* input_ctx = static_cast<xhci_input_context32*>(m_input_ctx);
        return &input_ctx->device_context.slot_context;
    }

    xhci_endpoint_context32* xhci_device::get_input_ctrl_ep_ctx() const {
        if (m_use64byte_ctx) {
            auto* input_ctx = static_cast<xhci_input_context64*>(m_input_ctx);
            return reinterpret_cast<xhci_endpoint_context32*>(&input_ctx->device_context.control_ep_context);
        }
        auto* input_ctx = static_cast<xhci_input_context32*>(m_input_ctx);
        return &input_ctx->device_context.control_ep_context;
    }

    xhci_endpoint_context32* xhci_device::get_input_ep_ctx(const uint8_t endpoint_num) const {
        uint8_t endpoint_index = endpoint_num - 2;

        if (m_use64byte_ctx) {
            auto* input_ctx = static_cast<xhci_input_context64*>(m_input_ctx);
            return reinterpret_cast<xhci_endpoint_context32*>(&input_ctx->device_context.ep[endpoint_index]);
        }
        auto* input_ctx = static_cast<xhci_input_context32*>(m_input_ctx);
        return &input_ctx->device_context.ep[endpoint_index];
    }

    void xhci_device::sync_input_ctx(const void* out_ctx) const {
        if (m_use64byte_ctx) {
            auto* input_ctx = static_cast<xhci_input_context64*>(m_input_ctx);
            xhci_device_context64* input_device_ctx = &input_ctx->device_context;
            std::memcpy(input_device_ctx, reinterpret_cast<u64 *>(to_virtual(out_ctx)), sizeof(xhci_device_context64));
        } else {
            auto* input_ctx = static_cast<xhci_input_context32*>(m_input_ctx);
            xhci_device_context32* input_device_ctx = &input_ctx->device_context;
            std::memcpy(input_device_ctx,  reinterpret_cast<u64 *>(to_virtual(out_ctx)), sizeof(xhci_device_context32));
        }
    }

    xhci_endpoint_context32* xhci_device::get_ep_ctx_by_dci(const uint8_t dci) const {
        const uint8_t index = dci - 2;
        if (m_use64byte_ctx) {
            auto* input_ctx = static_cast<xhci_input_context64*>(m_input_ctx);
            return reinterpret_cast<xhci_endpoint_context32*>(&input_ctx->device_context.ep[index]);
        }
        auto* input_ctx = static_cast<xhci_input_context32*>(m_input_ctx);
        return &input_ctx->device_context.ep[index];
    }
}
