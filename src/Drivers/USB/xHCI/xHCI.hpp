#pragma once

#include "xHCI_device.hpp"
#include "xHCI_regs.hpp"
#include "xHCI_rings.hpp"
#include "arch/x86_64/IDT/IDT.hpp"

namespace USB {

    constexpr uint16_t USB_DESCRIPTOR_REQUEST(uint8_t type, uint8_t index) {
        return (type << 8) | index;
    }

    constexpr uint8_t USB_DESCRIPTOR_DEVICE = 0x01;

    class xhci_driver {
    public:
        bool init_device();
        bool start_device();
        bool shutdown_device();

        void process_pending_port_changes();

        uintptr_t m_xhci_base;
    private:
        xhci_device* m_slot_devices[256] = {};
        uint32_t m_psc_pending_bitmap = 0;

        uint8_t irq_number;

        volatile xhci_capability_registers *m_cap_regs;
        volatile xhci_operational_registers *m_op_regs;
        volatile xhci_runtime_registers *m_runtime_regs;

        xhci_extended_capability *m_extended_capabilities_head = nullptr;

        // CAPLENGTH
        uint8_t m_capability_regs_length;

        // HCSPARAMS1
        uint8_t m_max_device_slots;
        uint8_t m_max_interrupters;
        uint8_t m_max_ports;

        // HCSPARAMS2
        uint8_t m_isochronous_scheduling_threshold;
        uint8_t m_erst_max;
        uint8_t m_max_scratchpad_buffers;

        // hccparams1
        bool m_64bit_addressing_capability;
        bool m_bandwidth_negotiation_capability;
        bool m_64byte_context_size;
        bool m_port_power_control;
        bool m_port_indicators;
        bool m_light_reset_capability;
        uint32_t m_extended_capabilities_offset;

        uint64_t* m_dcbaa;
        uint64_t* m_dcbaa_virtual;

        xhci_command_ring *m_command_ring = nullptr;
        xhci_event_ring *m_event_ring = nullptr;
        xhci_doorbell_manager *m_doorbell_manager = nullptr;

        std::vector<xhci_command_completion_trb_t*> m_command_completion_events;
        volatile uint8_t m_command_irq_completion = 0;
        struct xhci_pending_transfer {
            uint8_t  slot_id;
            uint8_t  endpoint_id; // DCI
            uint8_t  completion_code;
            uint32_t residual;    // bytes NOT transferred
        };
        std::vector<xhci_pending_transfer> m_pending_transfers;

        std::vector<uint8_t> m_usb3_ports;

        bool is_running = false;

    private:
        static void _xhci_irq_handler(const IDT::ISR_Registers *regs);
        static void _process_events();

        void _parse_capability_registers();
        void _parse_extended_capability_registers();
        xhci_usb_config _parse_config_descriptor(const uint8_t* buf, uint16_t total_length);

        // port number is 0-based
        static xhci_portsc_register _read_portsc_reg(uint8_t port_num);

        // port number is 0-based
        void _write_portsc_reg(xhci_portsc_register reg, uint8_t port_num);

        bool _is_usb3_port(uint8_t port_id) const;

        bool _reset_host_controller() const;
        bool _start_host_controller() const;

        void _configure_operational_register();
        void _setup_dcbaa();

        void _configure_runtime_registers();
        void _acknowledge_irq(uint8_t interrupter) const;

        xhci_command_completion_trb_t *_send_command_trb(xhci_trb_t* cmd_trb, uint32_t timeout = 500);

        // port number is 0-based
        bool _reset_port(uint8_t port_num);

        static const char* _usb_speed_to_string(uint8_t speed);
        static uint8_t _get_port_speed(uint8_t port);

        uint8_t _enable_device_slot();

        // Creates a device context buffer and inserts it into DCBAA
        bool _create_device_context(uint8_t slot_id) const;

        // port is 0-based
        void _setup_device(uint8_t port);
        void _enumerate_device(xhci_device *device);

        void _delete_device(uint8_t port);
        void _stop_device_endpoints(const xhci_device* device);
        void _stop_endpoint(uint8_t slot, uint8_t dci);
        void _disable_slot(uint8_t slot_id);

        static uint16_t _initial_max_packet_size(uint8_t speed);
        void _configure_ctrl_ep_input_context(xhci_device* device, uint16_t max_packet_size) const;

        void _address_device(const xhci_device* device, bool bsr);

        int32_t _get_device_descriptor(const xhci_device* device, void* out, uint16_t length);
        int32_t _get_config_descriptor(xhci_device* device, uint8_t config_index = 0);

        int32_t _send_control_transfer(const xhci_device* device,xhci_device_request_packet& request,void* buffer, uint32_t length, uint32_t timeout_ms = 1000);

        uint32_t _read_mfindex() const;

        void _evaluate_context(const xhci_device* device);

        int32_t _set_configuration(const xhci_device* device, uint8_t config_value);
        int32_t _configure_endpoint(xhci_device* device,const xhci_usb_endpoint& ep);
        int32_t _hid_set_idle(const xhci_device* device, uint8_t interface_num);
        int32_t _hid_set_protocol(const xhci_device* device, uint8_t interface_num, uint8_t protocol);
        void _init_hid_endpoint(xhci_device* device, const xhci_usb_endpoint& ep, xhci_hid_type type) const;
        void _arm_interrupt_in(const xhci_device* device, const xhci_hid_endpoint& hid) const;
        bool _wait_for_transfer(uint8_t slot, uint8_t dci, xhci_pending_transfer* out, uint32_t timeout_ms = 1000);
    };

    extern xhci_driver m_xhci_driver;
}
