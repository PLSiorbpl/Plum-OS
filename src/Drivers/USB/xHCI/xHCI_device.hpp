#pragma once
#include "xHCI_device_ctx.hpp"
#include "xHCI_rings.hpp"
#include "std/types.hpp"
#include "xHCI_device_config.hpp"

namespace USB {
    enum class xhci_hid_type : uint8_t {
        Unknown = 0,
        Keyboard = 1,
        Mouse = 2,
    };

    struct xhci_hid_endpoint {
        uint8_t   dci;
        uint8_t   slot;
        void*     buf_virt;
        uintptr_t buf_phys;
        uint8_t   max_packet_size;
        xhci_hid_type type;
    };

    class xhci_device {
    public:
        // port is a 1-based port ID
        xhci_device(uint8_t port, uint8_t slot, uint8_t speed, bool use_64byte_ctx);
        ~xhci_device() = default;

        [[nodiscard]] uint8_t get_port() const { return m_port; }
        [[nodiscard]] uint8_t get_port_regset_idx() const { return m_port - 1; }
        [[nodiscard]] uint8_t get_slot() const { return m_slot; }
        [[nodiscard]] uint8_t get_speed() const { return m_speed; }
        [[nodiscard]] uint32_t route_string() const { return m_route_string; }

        void set_root_port_id(const uint8_t rp) { m_root_port_id = rp; }
        void  set_output_ctx(void* ctx) { m_output_ctx = ctx; }
        [[nodiscard]] void* output_ctx() const { return m_output_ctx; }

        void set_active_config(xhci_usb_config& cfg) { m_active_config = std::move(cfg); }
        [[nodiscard]] const xhci_usb_config& active_config() const { return m_active_config; }
        [[nodiscard]] bool has_active_config() const { return m_active_config.config_value != 0; }

        [[nodiscard]] void* ctrl_transfer_buffer() const { return m_ctrl_transfer_buffer; }
        [[nodiscard]] uintptr_t ctrl_transfer_buffer_phys() const { return m_ctrl_transfer_buffer_phys; }

        [[nodiscard]] uintptr_t get_input_ctx_dma() const { return m_input_ctx_dma_addr; }

        [[nodiscard]] xhci_transfer_ring* ctrl_ring() const { return m_ctrl_ring; }

        [[nodiscard]] xhci_transfer_ring* get_ep_ring(const uint8_t dci) const {
            if (dci < 2 || dci > 31) return nullptr;
            return m_ep_rings[dci];
        }

        void set_ep_ring(const uint8_t dci, xhci_transfer_ring* ring) {
            if (dci < 2 || dci > 31) return;
            m_ep_rings[dci] = ring;
        }

        [[nodiscard]] xhci_input_control_context32* get_input_ctrl_ctx() const;
        [[nodiscard]] xhci_slot_context32* get_input_slot_ctx() const;
        [[nodiscard]] xhci_endpoint_context32* get_input_ctrl_ep_ctx() const;
        [[nodiscard]] xhci_endpoint_context32* get_input_ep_ctx(uint8_t endpoint_num) const;
        [[nodiscard]] xhci_endpoint_context32* get_ep_ctx_by_dci(uint8_t dci) const;

        void set_hid_ep(const xhci_hid_endpoint& ep) { m_hid_eps[ep.dci] = ep; }

        [[nodiscard]] const xhci_hid_endpoint* get_hid_ep(uint8_t dci) const {
            if (dci > 31) return nullptr;
            return m_hid_eps[dci].dci != 0 ? &m_hid_eps[dci] : nullptr;
        }

        [[nodiscard]] bool has_hid_ep(uint8_t dci) const {
            return dci <= 31 && m_hid_eps[dci].dci != 0;
        }

        void sync_input_ctx(const void* out_ctx) const;

    private:
        const uint8_t m_port = 0;           // 1-based port ID
        const uint8_t m_slot = 0;           // slot index in the xhci DCBAA
        const uint8_t m_speed = 0;          // port speed
        const bool    m_use64byte_ctx = false;

        xhci_hid_endpoint m_hid_eps[32] = {};

        uint32_t  m_route_string = 0;
        uint8_t   m_root_port_id = 0;

        void*     m_output_ctx = nullptr;
        xhci_usb_config m_active_config = {};
        xhci_transfer_ring* m_ep_rings[32] = {};

        void*         m_input_ctx = nullptr;
        uintptr_t     m_input_ctx_dma_addr = 0;

        void*     m_ctrl_transfer_buffer = nullptr;
        uintptr_t m_ctrl_transfer_buffer_phys = 0;

        xhci_transfer_ring* m_ctrl_ring = nullptr;

    private:
        void _alloc_input_ctx();
    };
}
