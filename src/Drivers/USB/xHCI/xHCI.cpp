#include "xHCI.hpp"

#include <kernel/Sleep.hpp>
#include <kernel/Memory/heap.hpp>

#include "xHCI_common.hpp"
#include "xHCI_mem.hpp"
#include "std/printf.hpp"
#include "Drivers/PCI.hpp"
#include "xHCI_trb.hpp"
#include "xHCI_rings.hpp"
#include "arch/x86_64/IDT/IDT.hpp"
#include "xHCI_ext_cap.hpp"
#include "xHCI_device_ctx.hpp"
#include "xHCI_device.hpp"
#include "kernel/log.h"
#include "structs.hpp"
#include "../mouse.hpp"
#include "../keyboard.hpp"

namespace USB {
    xhci_driver m_xhci_driver;

    bool xhci_driver::init_device() {
        const PCI::PCI_Device usb = PCI::Find_Class(0x0030030C);

        if (usb.vendor_id == 0) {
            log::error("[ xHCI ] No xHCI Device found");
            return false;
        }

        const uint32_t bar0 = usb.bar[0];
        const uint32_t bar1 = usb.bar[1];

        if (bar0 & 1) {
            log::error("[ xHCI ] xHCI should not use IO BAR");
            return false;
        }
        const uint32_t type = (bar0 >> 1) & 0b11;

        if (type == 0b00) { // 32bit addr
            m_xhci_base = bar0 & ~0xFULL;
        } else if (type == 0b10) { // 64bit addr
            m_xhci_base = (static_cast<uint64_t>(bar1) << 32) | (bar0 & ~0xFULL);
        }

        xhci_map_mmio(m_xhci_base, 0x10000);

        Time::Sleep(50);

        _parse_capability_registers();
        _parse_extended_capability_registers();

        if (!_reset_host_controller()) {
            return false;
        }

        _configure_operational_register();
        _configure_runtime_registers();

        irq_number = 10;
        PCI::install_interrupt(usb, _xhci_irq_handler, irq_number);

        return true;
    }

    bool xhci_driver::start_device() {
        if (!_start_host_controller()) {
            log::error("[ xHCI ] Failed to start the host controller");
            return false;
        }

        for (uint8_t port = 0; port < m_max_ports; port++) {
            xhci_portsc_register portsc = _read_portsc_reg(port);

            if (portsc.csc && portsc.ccs) {
                const bool reset_successful = _reset_port(port);

                portsc = _read_portsc_reg(port);
                if (reset_successful) {
                    log::success("[ xHCI ] Device connected on port #&a%u &f- %s", port, _usb_speed_to_string(portsc.port_speed));
                    _setup_device(port);
                } else {
                    log::error("[ xHCI ] Failed &fto reset port #&a%u &fafter connection detection", port);
                }
            }
        }

        is_running = true;
        return true;
    }

    void xhci_driver::process_pending_port_changes() {
        while (m_psc_pending_bitmap != 0) {
            const uint32_t bitmap = m_psc_pending_bitmap;
            m_psc_pending_bitmap = 0;

            for (uint8_t port = 0; port < m_max_ports && bitmap != 0; port++) {
                if (!(bitmap & (1u << port))) continue;

                xhci_portsc_register portsc = _read_portsc_reg(port);

                if (portsc.csc) {
                    if (portsc.ccs) {
                        const bool reset_successful = _reset_port(port);
                        portsc = _read_portsc_reg(port);

                        if (reset_successful) {
                            log::success("[ xHCI ] Device connected on port #&a%u &f- %s", port, _usb_speed_to_string(portsc.port_speed));
                            _setup_device(port);
                        } else {
                            log::error("[ xHCI ] Failed to reset device on port #&a%u", port);
                        }
                    } else {
                        log::info("[ xHCI ] Device disconnected from port %u", port);
                        _delete_device(port);

                        portsc.csc = 1;
                        portsc.pec = 1;
                        m_xhci_driver._write_portsc_reg(portsc, port);
                    }
                }
            }
        }
    }

    bool xhci_driver::shutdown_device() {
        is_running = false;
        return true;
    }

    void xhci_driver::_xhci_irq_handler(const IDT::ISR_Registers *regs) {
        _process_events();
        m_xhci_driver._acknowledge_irq(0);
    }

    void xhci_driver::_process_events() {
        std::vector<xhci_trb_t*> events;
        if (m_xhci_driver.m_event_ring->has_unprocessed_events()) {
            m_xhci_driver.m_event_ring->dequeue_events(events);
        }

        uint8_t command_completion_status = 0;

        for (const auto event : events) {
            switch (event->trb_type) {
                case XHCI_TRB_TYPE_CMD_COMPLETION_EVENT: {
                    command_completion_status = 1;
                    m_xhci_driver.m_command_completion_events.push_back(reinterpret_cast<xhci_command_completion_trb_t *>(event));
                    break;
                }
                case XHCI_TRB_TYPE_PORT_STATUS_CHANGE_EVENT: {
                    const auto* ev = reinterpret_cast<xhci_port_status_change_trb_t*>(event);
                    uint8_t port_id = ev->port_id;

                    if (port_id < 1 || port_id > m_xhci_driver.m_max_ports) {
                        log::warn("[ xHCI ] port status change for invalid port %u", port_id);
                        break;
                    }
                    const uint8_t port = port_id - 1;
                    if (port < 32) {
                        m_xhci_driver.m_psc_pending_bitmap |= (1u << port);
                    }
                    break;
                }
                case XHCI_TRB_TYPE_TRANSFER_EVENT: {
                    const auto* xfer = reinterpret_cast<xhci_transfer_event_trb_t*>(event);
                    const uint8_t slot = xfer->slot_id;
                    const uint8_t dci  = xfer->endpoint_id;

                    const xhci_device* dev = m_xhci_driver.m_slot_devices[slot];
                    if (dev && dev->has_hid_ep(dci)) {
                        const xhci_hid_endpoint& hid = *dev->get_hid_ep(dci);

                        if (xfer->completion_code == XHCI_TRB_COMPLETION_CODE_SUCCESS || xfer->completion_code == 13) {
                            const auto* report = static_cast<uint8_t*>(hid.buf_virt);

                            if (hid.type == xhci_hid_type::Mouse) {
                                mouse_state.buttons = report[0];
                                mouse_state.x += static_cast<int8_t>(report[1]);
                                mouse_state.y += static_cast<int8_t>(report[2]);
                                mouse_state.updated = true;

                            } else if (hid.type == xhci_hid_type::Keyboard) {
                                kb_push_key(report);
                            }
                        }

                        m_xhci_driver._arm_interrupt_in(dev, hid);

                    } else {
                        xhci_pending_transfer pending = {};
                        pending.slot_id         = slot;
                        pending.endpoint_id     = dci;
                        pending.completion_code = xfer->completion_code;
                        pending.residual        = xfer->trb_transfer_length;
                        m_xhci_driver.m_pending_transfers.push_back(pending);
                    }
                    break;
                }

                default: break;
            }
        }
        m_xhci_driver.m_command_irq_completion = command_completion_status;
    }

    bool xhci_driver::_wait_for_transfer(const uint8_t slot, const uint8_t dci, xhci_pending_transfer* out, const uint32_t timeout_ms) {
        uint32_t waited = 0;
        while (waited < timeout_ms) {
            _process_events();
            m_event_ring->flush_unprocessed_events();

            for (size_t i = 0; i < m_pending_transfers.size(); i++) {
                if (m_pending_transfers[i].slot_id == slot && m_pending_transfers[i].endpoint_id == dci) {
                    if (out) *out = m_pending_transfers[i];
                    m_pending_transfers.erase(i);
                    return true;
                }
            }
            Time::Sleep(10);
            waited += 10;
        }
        return false;
    }

    void xhci_driver::_arm_interrupt_in(const xhci_device* device, const xhci_hid_endpoint& hid) const {
        xhci_transfer_ring* ep_ring = device->get_ep_ring(hid.dci);
        if (!ep_ring) return;

        xhci_normal_trb_t trb = {};
        trb.trb_type = XHCI_TRB_TYPE_NORMAL;
        trb.data_buffer = hid.buf_phys;
        trb.trb_transfer_length = hid.max_packet_size;
        trb.td_size = 0;
        trb.interrupter_target = 0;
        trb.ioc = 1;
        trb.isp = 1;
        trb.chain = 0;

        ep_ring->enqueue(reinterpret_cast<xhci_trb_t*>(&trb));
        m_doorbell_manager->ring_doorbell(hid.slot, hid.dci);
    }

    void xhci_driver::_init_hid_endpoint(xhci_device* device, const xhci_usb_endpoint& ep, const xhci_hid_type type) const {
        void* buf_virt = alloc_xhci_memory(ep.max_packet_size);
        const uintptr_t buf_phys = xhci_get_physical_addr(buf_virt);

        xhci_hid_endpoint hid = {};
        hid.dci = ep.dci;
        hid.slot = device->get_slot();
        hid.buf_virt = buf_virt;
        hid.buf_phys = buf_phys;
        hid.max_packet_size = ep.max_packet_size;
        hid.type = type;

        device->set_hid_ep(hid);
        _arm_interrupt_in(device, hid);

        const char* type_str = (type == xhci_hid_type::Mouse) ? "Mouse" : (type == xhci_hid_type::Keyboard) ? "Keyboard" : "HID";
        log::success("[ HID ] armed %s DCI=%u slot=%u", type_str, ep.dci, device->get_slot());
    }

    void xhci_driver::_parse_capability_registers() {
        m_cap_regs = reinterpret_cast<volatile xhci_capability_registers *>(m_xhci_base);
        m_capability_regs_length = m_cap_regs->caplength;
        m_max_device_slots = XHCI_MAX_DEVICE_SLOTS(m_cap_regs);
        m_max_interrupters = XHCI_MAX_INTERRUPTERS(m_cap_regs);
        m_max_ports = XHCI_MAX_PORTS(m_cap_regs);
        m_isochronous_scheduling_threshold = XHCI_IST(m_cap_regs);
        m_erst_max = XHCI_ERST_MAX(m_cap_regs);
        m_max_scratchpad_buffers = XHCI_MAX_SCRATCHPAD_BUFFERS(m_cap_regs);
        m_64bit_addressing_capability = XHCI_AC64(m_cap_regs);
        m_bandwidth_negotiation_capability = XHCI_BNC(m_cap_regs);
        m_64byte_context_size = XHCI_CSZ(m_cap_regs);
        m_port_power_control = XHCI_PPC(m_cap_regs);
        m_port_indicators = XHCI_PIND(m_cap_regs);
        m_light_reset_capability = XHCI_LHRC(m_cap_regs);
        m_extended_capabilities_offset = XHCI_XECP(m_cap_regs) * sizeof(uint32_t);

        m_op_regs = reinterpret_cast<volatile xhci_operational_registers*>(m_xhci_base + m_capability_regs_length);
        m_runtime_regs = reinterpret_cast<volatile xhci_runtime_registers*>(m_xhci_base + m_cap_regs->rtsoff);
        m_doorbell_manager = new xhci_doorbell_manager(m_xhci_base + m_cap_regs->dboff);
    }

    void xhci_driver::_parse_extended_capability_registers() {
        volatile auto* head_cap_ptr = reinterpret_cast<volatile uint32_t*>(
            m_xhci_base + m_extended_capabilities_offset
        );

        m_extended_capabilities_head = new xhci_extended_capability(head_cap_ptr);
        auto node = m_extended_capabilities_head;

        while (node) {
            if (node->id() == xhci_extended_capability_code::supported_protocol) {
                const xhci_usb_supported_protocol_capability cap(node->base());

                const uint8_t first_port = cap.compatible_port_offset - 1;
                const uint8_t last_port = first_port + cap.compatible_port_count - 1;

                if (cap.major_revision_version == 3) {
                    for (uint8_t port = first_port; port <= last_port; port++) {
                        m_usb3_ports.push_back(port);
                    }
                }
            }

            node = node->next();
        }
    }

    xhci_portsc_register xhci_driver::_read_portsc_reg(const uint8_t port_num) {
        const uint64_t reg_base = reinterpret_cast<uint64_t>(m_xhci_driver.m_op_regs) + (0x400 + (0x10 * port_num));

        xhci_portsc_register reg = {};
        reg.raw = *reinterpret_cast<volatile uint32_t *>(reg_base);

        return reg;
    }

    void xhci_driver::_write_portsc_reg(const xhci_portsc_register reg, const uint8_t port_num) {
        const uint64_t reg_base = reinterpret_cast<uint64_t>(m_op_regs) + (0x400 + (0x10 * port_num));
        *reinterpret_cast<volatile uint32_t *>(reg_base) = reg.raw;
    }

    bool xhci_driver::_is_usb3_port(const uint8_t port_id) const {
        for (const unsigned char m_usb3_port : m_usb3_ports) {
            if (m_usb3_port == port_id) {
                return true;
            }
        }
        return false;
    }

    bool xhci_driver::_reset_host_controller() const {
        // Clear Run/Stop bit
        uint32_t usbcmd = m_op_regs->usbcmd;
        usbcmd &= ~XHCI_USBCMD_RUN_STOP;
        m_op_regs->usbcmd = usbcmd;

        // Wait for HCHalted bit to be set
        uint32_t timeout = 20;
        while (!(m_op_regs->usbsts & XHCI_USBSTS_HCH)) {
            if (--timeout <= 0) {
                log::error("[ xHCI ] Host controller did not halt within 200ms");
                return false;
            }
            Time::Sleep(10);
        }

        // Set reset bit
        usbcmd = m_op_regs->usbcmd;
        usbcmd |= XHCI_USBCMD_HCRESET;
        m_op_regs->usbcmd = usbcmd;

        // Wait for Reset bit and CNR bit to clear
        timeout = 50;
        while (m_op_regs->usbcmd & XHCI_USBCMD_HCRESET || m_op_regs->usbsts & XHCI_USBSTS_CNR) {
            if (--timeout == 0) {
                log::error("[ xHCI ] Host controller did not reset within 500ms");
                return false;
            }

            Time::Sleep(10);
        }

        Time::Sleep(50);

        if (m_op_regs->usbcmd != 0 || m_op_regs->dnctrl != 0)
            return false;
        if (m_op_regs->crcr != 0 || m_op_regs->dcbaap != 0)
            return false;
        if (m_op_regs->config != 0)
            return false;

        return true;
    }

    bool xhci_driver::_start_host_controller() const {
        uint32_t usbcmd = m_op_regs->usbcmd;
        usbcmd |= XHCI_USBCMD_RUN_STOP;
        usbcmd |= XHCI_USBCMD_INTERRUPTER_ENABLE;
        m_op_regs->usbcmd = usbcmd;

        uint32_t timeout = 50;
        while (m_op_regs->usbsts & XHCI_USBSTS_HCH) {
            if (--timeout == 0) {
                log::error("[ xHCI ] Host controller did not halt within 500ms");
                return false;
            }
            Time::Sleep(10);
        }

        if (m_op_regs->usbsts & XHCI_USBSTS_CNR)
            return false;

        return true;
    }

    void xhci_driver::_configure_operational_register() {
        m_op_regs->dnctrl = 0xffff;
        m_op_regs->config = static_cast<uint32_t>(m_max_device_slots);

        _setup_dcbaa();

        m_command_ring = new xhci_command_ring(XHCI_COMMAND_RING_TRB_COUNT);
        m_op_regs->crcr = m_command_ring->get_physical_base() | m_command_ring->get_cycle_bit();
    }

    void xhci_driver::_setup_dcbaa() {
        const uint64_t dcbaa_size = sizeof(uintptr_t) * (m_max_device_slots + 1);

        m_dcbaa = static_cast<uint64_t *>(alloc_xhci_memory(dcbaa_size, XHCI_DEVICE_CONTEXT_ALIGNMENT, XHCI_DEVICE_CONTEXT_BOUNDARY));
        m_dcbaa_virtual = static_cast<uint64_t *>(heap::malloc((m_max_device_slots + 1) * sizeof(uint64_t)));

        if (m_max_scratchpad_buffers > 0) {
            auto *scrachpad_array = static_cast<uint64_t *>(
                alloc_xhci_memory(m_max_scratchpad_buffers * sizeof(uint64_t), XHCI_DEVICE_CONTEXT_ALIGNMENT, XHCI_DEVICE_CONTEXT_BOUNDARY));

            for (uint32_t i = 0; i < m_max_scratchpad_buffers; i++) {
                void *scrachpad = alloc_xhci_memory(PAGE_SIZE, XHCI_SCRATCHPAD_BUFFERS_ALIGNMENT, XHCI_SCRATCHPAD_BUFFERS_BOUNDARY);
                const uint64_t scrachpad_addr = xhci_get_physical_addr(scrachpad);
                scrachpad_array[i] = scrachpad_addr;
            }

            const uint64_t scrachpad_array_physical_base = xhci_get_physical_addr(scrachpad_array);
            m_dcbaa[0] = scrachpad_array_physical_base;

            m_dcbaa_virtual[0] = scrachpad_array_physical_base;
        }

        m_op_regs->dcbaap = xhci_get_physical_addr(m_dcbaa);
    }

    void xhci_driver::_configure_runtime_registers() {
        volatile xhci_interrupter_registers* interrupter_regs = &m_runtime_regs->ir[0];

        // Enable interrupts
        uint32_t iman = interrupter_regs->iman;
        iman |= XHCI_IMAN_INTERRUPT_ENABLE;
        interrupter_regs->iman = iman;

        interrupter_regs->imod = 0;

        m_event_ring = new xhci_event_ring(XHCI_EVENT_RING_TRB_COUNT, interrupter_regs);
        _acknowledge_irq(0);
    }

    void xhci_driver::_acknowledge_irq(const uint8_t interrupter) const {
        m_op_regs->usbsts = XHCI_USBSTS_EINT;

        volatile xhci_interrupter_registers* interrupter_regs = &m_runtime_regs->ir[interrupter];

        uint32_t iman = interrupter_regs->iman;
        iman |= XHCI_IMAN_INTERRUPT_PENDING;
        interrupter_regs->iman = iman;
    }

    xhci_command_completion_trb_t *xhci_driver::_send_command_trb(xhci_trb_t* cmd_trb, const uint32_t timeout) {
        m_command_ring->enqueue(cmd_trb);
        m_doorbell_manager->ring_command_doorbell();

        uint64_t m_timeout = timeout/10;
        while (!m_command_irq_completion) {
            if (--m_timeout == 0) {
                log::error("[ xHCI ] Send command trb Timed out in %ums", timeout);
                break;
            }
            Time::Sleep(10);
        }

        xhci_command_completion_trb_t* completion_trb = !m_command_completion_events.empty() ? m_command_completion_events[0] : nullptr;

        m_command_completion_events.clear();
        m_command_irq_completion = 0;

        if (!completion_trb) {
            std::kernel::printf("Failed to find completion TRB for command %i", cmd_trb->trb_type);
            return nullptr;
        }

        if (completion_trb->completion_code != XHCI_TRB_COMPLETION_CODE_SUCCESS) {
            log::error("[ xHCI ] Command TRB failed with error: %s", trb_completion_code_to_string(completion_trb->completion_code));
            return nullptr;
        }

        return completion_trb;
    }

    bool xhci_driver::_reset_port(uint8_t port_num) {
        xhci_portsc_register portsc = _read_portsc_reg(port_num);

        bool is_usb_3_port = _is_usb3_port(port_num);

        if (portsc.pp == 0) {
            portsc.pp = 1;
            _write_portsc_reg(portsc, port_num);
            Time::Sleep(20);
            portsc = _read_portsc_reg(port_num);

            if (portsc.pp == 0) {
                log::error("[ xHCI ] &cPort #&a%u &cFailed to power", port_num);
                return false;
            }
        }

        portsc.csc = 1;
        portsc.pec = 1;
        portsc.prc = 1;
        _write_portsc_reg(portsc, port_num);
        if (is_usb_3_port) {
            portsc.wpr = 1; // Usb 3 warm port reset (faster)
        } else {
            portsc.pr = 1; // Usb 2 port reset (slower)
        }
        _write_portsc_reg(portsc, port_num);
        int timeout = 10;
        while (timeout > 0) {
            portsc = _read_portsc_reg(port_num);

            if ((is_usb_3_port && portsc.wrc) || (!is_usb_3_port && portsc.prc)) {
                break;
            }
            timeout--;
            Time::Sleep(10);
        }

        if (timeout == 0) {
            log::error("[ xHCI ] Port #%au Port reset failed timed out in 100ms", port_num);
            return false;
        }
        Time::Sleep(10); // to stabilize controller

        portsc.prc = 1; // Clear port reset change
        portsc.wrc = 1; // Clear warm reset change (USB 3.0)
        portsc.csc = 1; // Clear connect status change
        portsc.pec = 1; // Clear port enable/disable change
        portsc.ped = 0; // Don't clear the PED bit
        _write_portsc_reg(portsc, port_num);

        Time::Sleep(10); // to stabilize controller

        portsc = _read_portsc_reg(port_num);

        if (portsc.ped == 0) {
            return false;
        }

        return true;
    }

    const char *xhci_driver::_usb_speed_to_string(const uint8_t speed) {
        static const char* speed_string[7] = {
            "Invalid",
            "Full Speed (12 MB/s - USB2.0)",
            "Low Speed (1.5 Mb/s - USB 2.0)",
            "High Speed (480 Mb/s - USB 2.0)",
            "Super Speed (5 Gb/s - USB3.0)",
            "Super Speed Plus (10 Gb/s - USB 3.1)",
            "Undefined"
        };
        return speed_string[speed];
    }

    uint8_t xhci_driver::_get_port_speed(const uint8_t port) {
        const xhci_portsc_register portsc = _read_portsc_reg(port);
        return static_cast<uint8_t>(portsc.port_speed);
    }

    uint8_t xhci_driver::_enable_device_slot() {
        xhci_trb_t enable_slot_trb = {};
        enable_slot_trb.trb_type = XHCI_TRB_TYPE_ENABLE_SLOT_CMD;

        const auto completion_trb = _send_command_trb(&enable_slot_trb);
        if (!completion_trb) {
            return 0;
        }

        return completion_trb->slot_id;
    }

    bool xhci_driver::_create_device_context(const uint8_t slot_id) const {
        const uint64_t device_context_size = m_64byte_context_size ? sizeof(xhci_device_context64) : sizeof(xhci_device_context32);
        void* ctx = alloc_xhci_memory(device_context_size,XHCI_DEVICE_CONTEXT_ALIGNMENT,XHCI_DEVICE_CONTEXT_BOUNDARY);

        if (!ctx) {
            log::error("[ xHCI ] Failed to allocate memory for a device context");
            return false;
        }

        m_dcbaa[slot_id] = xhci_get_physical_addr(ctx);
        m_dcbaa_virtual[slot_id] = reinterpret_cast<uint64_t>(ctx);

        return true;
    }

    void xhci_driver::_delete_device(uint8_t port) {
        const xhci_device* device = nullptr;
        uint8_t slot_id = 0;

        for (uint8_t s = 1; s <= m_max_device_slots; s++) {
            if (m_slot_devices[s] && m_slot_devices[s]->get_port() == port + 1) {
                device = m_slot_devices[s];
                slot_id = s;
                break;
            }
        }

        if (!device) {
            log::warn("[ xHCI ] No device on port %u", port);
            return;
        }

        _stop_device_endpoints(device);
        _disable_slot(slot_id);

        for (uint8_t dci = 2; dci < 32; dci++) {
            if (!device->has_hid_ep(dci)) continue;
            const xhci_hid_endpoint& hid = *device->get_hid_ep(dci);

            if (hid.type == xhci_hid_type::Mouse) {
                mouse_state = {};
                log::info("[ xHCI ] Mouse removed");
            } else if (hid.type == xhci_hid_type::Keyboard) {
                kb_state = {};
                log::info("[ xHCI ] Keyboard removed");
            }
        }

        m_slot_devices[slot_id] = nullptr;
        m_dcbaa[slot_id] = 0;
        m_dcbaa_virtual[slot_id] = 0;

        for (size_t i = 0; i < m_pending_transfers.size(); ) {
            if (m_pending_transfers[i].slot_id == slot_id) {
                m_pending_transfers.erase(i);
            } else {
                i++;
            }
        }

        delete device;

        log::success("[ xHCI ] slot %u removed cleanly", slot_id);
    }

    void xhci_driver::_stop_device_endpoints(const xhci_device* device) {
        for (uint8_t dci = 1; dci < 32; dci++) {
            xhci_transfer_ring* ring = (dci == 1)
                ? device->ctrl_ring()
                : device->get_ep_ring(dci);
            if (!ring) continue;

            _stop_endpoint(device->get_slot(), dci);
        }
    }

    void xhci_driver::_stop_endpoint(const uint8_t slot, const uint8_t dci) {
        xhci_stop_endpoint_command_trb_t trb = {};
        trb.trb_type = XHCI_TRB_TYPE_STOP_ENDPOINT_CMD;
        trb.endpoint_id = dci;
        trb.slot_id = slot;

        if (!_send_command_trb(reinterpret_cast<xhci_trb_t *>(&trb))) {
            log::error("[ xHCI ] Stop endpoint failed for slot %u", slot);
        }
    }

    void xhci_driver::_disable_slot(uint8_t slot_id) {
        xhci_disable_slot_command_trb_t trb = {};
        trb.trb_type = XHCI_TRB_TYPE_DISABLE_SLOT_CMD;
        trb.slot_id = slot_id;

        if (!_send_command_trb(reinterpret_cast<xhci_trb_t *>(&trb))) {
            log::error("[ xHCI ] Disable Slot failed for slot %u", slot_id);
        }
    }

    void xhci_driver::_setup_device(uint8_t port) {
        const uint8_t port_speed = _get_port_speed(port);
        const uint8_t port_id = port + 1;

        uint8_t slot_id = _enable_device_slot();
        if (!slot_id) {
            log::error("[ xHCI ] Failed to enable device slot for port &a%i", port);
            return;
        }

        if (!_create_device_context(slot_id)) {
            log::error("[ xHCI ] Failed to create device context for slot &a%i", slot_id);
            return;
        }

        auto* device = new xhci_device(port_id, slot_id, port_speed, m_64byte_context_size);
        m_slot_devices[slot_id] = device;

        device->set_root_port_id(port_id);
        device->set_output_ctx(reinterpret_cast<void *>(m_dcbaa[slot_id]));

        _enumerate_device(device);
    }

    void xhci_driver::_enumerate_device(xhci_device *device) {
        uint8_t slot_id = device->get_slot();
        const uint8_t port_speed = device->get_speed();

        const uint16_t max_packet_size = _initial_max_packet_size(port_speed);
        _configure_ctrl_ep_input_context(device, max_packet_size);

        // BRS 1
        _address_device(device, true);
        Time::Sleep(50);

        usb_device_descriptor desc = {};
        if (_get_device_descriptor(device, &desc, 8) != 0) {
            log::error("[ xHCI ] Failed to read partial device descriptor for slot %u", slot_id);
            return;
        }

        if (port_speed == XHCI_USB_SPEED_LOW_SPEED || port_speed == XHCI_USB_SPEED_FULL_SPEED) {
            if (desc.bMaxPacketSize0 != max_packet_size) {
                device->get_input_ctrl_ep_ctx()->max_packet_size = desc.bMaxPacketSize0;
                device->get_input_ctrl_ctx()->add_flags = (1 << 1); // A1 only
                device->get_input_ctrl_ctx()->drop_flags = 0;
                _evaluate_context(device);
            }
        }

        // BRS 0
        _address_device(device, false);
        Time::Sleep(50);

        if (_get_device_descriptor(device, &desc, sizeof(desc)) != 0) {
            log::error("[ xHCI ] Failed to read full device descriptor for slot %u", slot_id);
            return;
        }

        _get_config_descriptor(device, 0);

        if (!device->has_active_config()) {
            log::error("[ xHCI ] No config parsed for slot %u", slot_id);
            return;
        }

        const xhci_usb_config& config = device->active_config();

        for (const auto & iface : config.interfaces) {
            log::info("  iface %u: class=%x protocol=%x endpoints=%u", iface.number, iface.class_code, iface.protocol, iface.endpoints.size());
        }

        _set_configuration(device, config.config_value);

        for (const auto & iface : config.interfaces) {
            for (auto ep : iface.endpoints) {
                if (_configure_endpoint(device, ep) != 0) {
                    log::error("[ xHCI ] failed to configure endpoint %x for slot %u", ep.address, device->get_slot());
                    return;
                }
            }
        }
        for (const auto & iface : config.interfaces) {
            if (iface.class_code == USB_CLASS_HID) {
                _hid_set_idle(device, iface.number);
                _hid_set_protocol(device, iface.number, 0);

                const xhci_hid_type hid_type = (iface.protocol == 1) ? xhci_hid_type::Keyboard : (iface.protocol == 2) ? xhci_hid_type::Mouse : xhci_hid_type::Unknown;

                for (auto ep : iface.endpoints) {
                    if (ep.type == 0x03 && ep.is_in) {
                        _init_hid_endpoint(device, ep, hid_type);
                        break;
                    }
                }
            }
        }

    }

    uint16_t xhci_driver::_initial_max_packet_size(const uint8_t speed) {
        switch (speed) {
            case XHCI_USB_SPEED_LOW_SPEED:
                return 8;
            case XHCI_USB_SPEED_FULL_SPEED:
            case XHCI_USB_SPEED_HIGH_SPEED:
                return 64;
            case XHCI_USB_SPEED_SUPER_SPEED:
            case XHCI_USB_SPEED_SUPER_SPEED_PLUS:
                return 512;
            default:
                return 8;
        }
    }

    void xhci_driver::_evaluate_context(const xhci_device* device) {
        xhci_evaluate_context_command_trb_t eval_trb = {};
        eval_trb.input_context_physical_base = device->get_input_ctx_dma();
        eval_trb.trb_type = XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD;
        eval_trb.slot_id = device->get_slot();

        _send_command_trb(reinterpret_cast<xhci_trb_t*>(&eval_trb));
    }

    void xhci_driver::_configure_ctrl_ep_input_context(xhci_device* device, const uint16_t max_packet_size) const {
        const size_t ctx_size = XHCI_CSZ(m_cap_regs) ? sizeof(xhci_input_context64) : sizeof(xhci_input_context32);
        std::memset(device->get_input_ctrl_ctx(), 0, ctx_size);

        auto* input_ctrl = device->get_input_ctrl_ctx();
        auto* slot_ctx = device->get_input_slot_ctx();
        auto* ep0_ctx = device->get_input_ctrl_ep_ctx();

        // Enable slot context (A0) and control endpoint context (A1)
        input_ctrl->add_flags = (1 << 0) | (1 << 1);
        input_ctrl->drop_flags = 0;

        slot_ctx->route_string = device->route_string();
        slot_ctx->speed = device->get_speed();
        slot_ctx->context_entries = 1;
        slot_ctx->interrupter_target = 0;

        if (device->route_string() == 0) {
            // Root hub device
            slot_ctx->root_hub_port_num = device->get_port();
        } else {
            log::error("[ xHCI ] skill issue #1");
            //// Hub-downstream device: use the root port of the topology chain
            //slot_ctx->root_hub_port_num = device->root_port_id();
            //
            //// xHCI spec Section 6.2.2: parent_hub_slot_id and parent_port_number
            //// shall reference the nearest HS hub providing the TT, only for LS/FS
            //// devices. Walk up the hub chain to find it.
            //if (device->get_speed() == XHCI_USB_SPEED_LOW_SPEED ||
            //    device->get_speed() == XHCI_USB_SPEED_FULL_SPEED) {
            //    auto* hub = m_slot_devices[device->parent_slot_id()];
            //    uint8_t port_on_hub = device->parent_port_num();
            //    while (hub && hub->speed() != XHCI_USB_SPEED_HIGH_SPEED
            //               && hub->parent_slot_id() != 0) {
            //        port_on_hub = hub->parent_port_num();
            //        hub = m_slot_devices[hub->parent_slot_id()];
            //    }
            //    if (hub && hub->speed() == XHCI_USB_SPEED_HIGH_SPEED) {
            //        slot_ctx->parent_hub_slot_id = hub->slot_id();
            //        slot_ctx->parent_port_number = port_on_hub;
            //        slot_ctx->mtt = hub->mtt() ? 1 : 0;
            //        log::info("[ xHCI ] slot %u TT: hub_slot=%u hub_port=%u mtt=%u",
            //                  device->slot_id(), hub->slot_id(), port_on_hub,
            //                  hub->mtt() ? 1 : 0);
            //    } else {
            //        log::info("[ xHCI ] slot %u TT: no HS hub found (parent slot %u speed=%u)",
            //                  device->slot_id(), device->parent_slot_id(),
            //                  hub ? hub->speed() : 0xFF);
            //    }
            //}
        }

        ep0_ctx->endpoint_state = XHCI_ENDPOINT_STATE_DISABLED;
        ep0_ctx->endpoint_type = XHCI_ENDPOINT_TYPE_CONTROL;
        ep0_ctx->max_packet_size = max_packet_size;
        ep0_ctx->max_burst_size = 0;
        ep0_ctx->error_count = 3;
        ep0_ctx->interval = 0;
        ep0_ctx->average_trb_length = 8;
        ep0_ctx->max_esit_payload_lo = 0;
        ep0_ctx->transfer_ring_dequeue_ptr = device->ctrl_ring()->get_physical_base();
        ep0_ctx->dcs = device->ctrl_ring()->get_cycle_bit();
    }

    void xhci_driver::_address_device(const xhci_device* device, const bool bsr) {
        xhci_address_device_command_trb_t address_trb = {};
        address_trb.input_context_physical_base = device->get_input_ctx_dma();
        address_trb.bsr = bsr ? 1 : 0;
        address_trb.trb_type = XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD;
        address_trb.slot_id = device->get_slot();

        _send_command_trb(reinterpret_cast<xhci_trb_t*>(&address_trb));
    }

    int32_t xhci_driver::_get_device_descriptor(const xhci_device* device, void* out, const uint16_t length) {
        xhci_device_request_packet req = {};
        req.bRequestType = 0x80;
        req.bRequest = 6;
        req.wValue = USB_DESCRIPTOR_REQUEST(USB_DESCRIPTOR_DEVICE, 0);
        req.wIndex = 0;
        req.wLength = length;

        return _send_control_transfer(device, req, out, length);
    }

    int32_t xhci_driver::_get_config_descriptor(xhci_device *device, const uint8_t config_index) {
        uint8_t slot_id = device->get_slot();

        usb_configuration_descriptor header = {};
        {
            xhci_device_request_packet req = {};
            req.bRequestType = 0x80;
            req.bRequest = 0x06;
            req.wValue = USB_DESCRIPTOR_REQUEST(USB_DESCRIPTOR_CONFIGURATION, config_index);
            req.wIndex = 0;
            req.wLength = sizeof(usb_configuration_descriptor);

            if (_send_control_transfer(device, req, &header, sizeof(header)) != 0) {
                log::error("[ xHCI ] failed to fetch config descriptor header for slot %u", slot_id);
                return -1;
            }
        }

        if (header.bDescriptorType != USB_DESCRIPTOR_CONFIGURATION) {
            log::error("[ xHCI ] got descriptor type %x instead of config for slot %u", header.bDescriptorType, slot_id);
            log::info("[ xHCI ] config header raw bytes:");
            uint8_t* raw = reinterpret_cast<uint8_t*>(&header);
            for (int i = 0; i < 9; i++)
                log::info("  [%d] = %x", i, raw[i]);
            return -1;
        }

        uint16_t total_length = header.wTotalLength;
        if (total_length < sizeof(usb_configuration_descriptor) || total_length > 512) {
            log::error("[ xHCI ] bogus wTotalLength=%u for slot %u", total_length, slot_id);
            return -1;
        }

        auto* buf = static_cast<uint8_t*>(heap::malloc(total_length));
        std::memset(buf, 0, total_length);

        {
            xhci_device_request_packet req = {};
            req.bRequestType = 0x80;
            req.bRequest = 0x06;
            req.wValue = USB_DESCRIPTOR_REQUEST(USB_DESCRIPTOR_CONFIGURATION, config_index);
            req.wIndex = 0;
            req.wLength = total_length;

            if (_send_control_transfer(device, req, buf, total_length) != 0) {
                log::error("[ xHCI ] failed to fetch full config descriptor for slot %u", slot_id);
                heap::free(buf);
                return -1;
            }
        }

        xhci_usb_config parsed = _parse_config_descriptor(buf, total_length);
        device->set_active_config(parsed);

        heap::free(buf);
        return 0;
    }


    int32_t xhci_driver::_send_control_transfer(const xhci_device* device, xhci_device_request_packet& request, void* buffer, uint32_t length, const uint32_t timeout_ms) const {
        xhci_transfer_ring* ring = device->ctrl_ring();

        void* dma_buffer = device->ctrl_transfer_buffer();
        const uintptr_t dma_buffer_phys = device->ctrl_transfer_buffer_phys();
        if (!dma_buffer || dma_buffer_phys == 0) {
            log::error("[ xHCI ] missing control transfer buffer for slot %u", device->get_slot());
            return -1;
        }
        if (length > PAGE_SIZE) {
            log::error("[ xHCI ] control transfer too large (%u bytes)", length);
            return -1;
        }

        const bool is_in = (request.transfer_direction != 0);

        if (length > 0 && !is_in && buffer) {
            std::memcpy(dma_buffer, buffer, length);
        } else {
            std::memset(dma_buffer, 0, length > 0 ? length : 1);
        }

        // Setup Stage TRB
        xhci_setup_stage_trb_t setup = {};
        setup.trb_type = XHCI_TRB_TYPE_SETUP_STAGE;
        setup.request_packet = request;
        setup.trb_transfer_length = 8;
        setup.interrupter_target = 0;
        setup.idt = 1;
        setup.ioc = 0;
        // TRT: 0=No Data, 2=OUT Data, 3=IN Data
        setup.trt = (length > 0) ? (is_in ? 3 : 2) : 0;

        // Data Stage TRB
        xhci_data_stage_trb_t data = {};
        if (length > 0) {
            data.trb_type = XHCI_TRB_TYPE_DATA_STAGE;
            data.data_buffer = dma_buffer_phys;
            data.trb_transfer_length = length;
            data.td_size = 0;
            data.interrupter_target = 0;
            data.dir = is_in ? 1 : 0;
            data.ioc = 0;
            data.idt = 0;
            data.chain = 0;
        }

        // Status Stage TRB
        xhci_status_stage_trb_t status = {};
        status.trb_type = XHCI_TRB_TYPE_STATUS_STAGE;
        status.interrupter_target = 0;
        status.ioc = 1; // Interrupt
        status.dir = (length > 0) ? (is_in ? 0 : 1) : 1;

        //device->set_ctrl_completed(false);

        ring->enqueue(reinterpret_cast<xhci_trb_t*>(&setup));
        if (length > 0) {
            ring->enqueue(reinterpret_cast<xhci_trb_t*>(&data));
        }
        ring->enqueue(reinterpret_cast<xhci_trb_t*>(&status));

        if (device->route_string() != 0 && device->get_speed() == XHCI_USB_SPEED_FULL_SPEED) {
            for (uint32_t tries = 0; tries < 50; tries++) {
                if ((_read_mfindex() & 0x7) != 0)
                    break;
                Time::Sleep(10);
            }
        }

        m_doorbell_manager->ring_doorbell(device->get_slot(), XHCI_DOORBELL_TARGET_CONTROL_EP_RING);

        constexpr uint64_t XFER_TIMEOUT_MS = 5;
        const uint64_t deadline = Time::tick + XFER_TIMEOUT_MS * 1ULL;

        _process_events();
        m_event_ring->flush_unprocessed_events();

        while (Time::tick < deadline) {
            Time::Sleep(50);
            _process_events();
            m_event_ring->flush_unprocessed_events();
        }

        if (buffer && length > 0 && is_in) {
            std::memcpy(buffer, dma_buffer, length);
        }

        return 0;
    }

    uint32_t xhci_driver::_read_mfindex() const {
        if (!m_runtime_regs) {
            return 0xffffu;
        }
        return m_runtime_regs->mf_index & 0x3fff;
    }

    int32_t xhci_driver::_set_configuration(const xhci_device* device, const uint8_t config_value) {
        xhci_device_request_packet req = {};
        req.bRequestType = 0x00;
        req.bRequest = 0x09;
        req.wValue = config_value;
        req.wIndex = 0;
        req.wLength = 0;

        if (_send_control_transfer(device, req, nullptr, 0) != 0) {
            log::error("[ xHCI ] SET_CONFIGURATION failed for slot %u", device->get_slot());
            return -1;
        }

        return 0;
    }

    int32_t xhci_driver::_configure_endpoint(xhci_device* device, const xhci_usb_endpoint& ep) {
        uint8_t dci = ep.dci;

        auto* ep_ring = new xhci_transfer_ring();
        if (ep_ring->init(XHCI_TRANSFER_RING_TRB_COUNT, device->get_slot()) != 0) {
            log::error("[ xHCI ] failed to alloc transfer ring for DCI %u slot %u",
                       dci, device->get_slot());
            delete ep_ring;
            return -1;
        }
        device->set_ep_ring(dci, ep_ring);

        device->sync_input_ctx(device->output_ctx());

        auto* input_ctrl = device->get_input_ctrl_ctx();
        input_ctrl->add_flags = (1 << 0) | (1 << dci);
        input_ctrl->drop_flags = 0;

        auto* slot_ctx = device->get_input_slot_ctx();
        if (dci > slot_ctx->context_entries)
            slot_ctx->context_entries = dci;

        //   1=Isoch OUT  2=Bulk OUT  3=Interrupt OUT
        //   4=Control    5=Isoch IN  6=Bulk IN  7=Interrupt IN
        uint8_t xhci_ep_type;
        switch (ep.type) {
            case 0x01: xhci_ep_type = ep.is_in ? 5 : 1; break; // Isochronous
            case 0x02: xhci_ep_type = ep.is_in ? 6 : 2; break; // Bulk
            case 0x03: xhci_ep_type = ep.is_in ? 7 : 3; break; // Interrupt
            default: xhci_ep_type = 4; break; // Control
        }

        auto* ep_ctx = device->get_ep_ctx_by_dci(dci);
        std::memset(ep_ctx, 0, sizeof(xhci_endpoint_context32));

        ep_ctx->endpoint_type = xhci_ep_type;
        ep_ctx->max_packet_size = ep.max_packet_size;
        ep_ctx->max_burst_size = 0;
        ep_ctx->error_count = (ep.type == 0x01) ? 0 : 3; // 0 = isochronous
        ep_ctx->interval = ep.interval - 1;
        ep_ctx->average_trb_length = ep.max_packet_size;
        ep_ctx->transfer_ring_dequeue_ptr = ep_ring->get_physical_base();
        ep_ctx->dcs              = ep_ring->get_cycle_bit();

        xhci_configure_endpoint_command_trb_t cfg_trb = {};
        cfg_trb.input_context_physical_base = device->get_input_ctx_dma();
        cfg_trb.trb_type = XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD;
        cfg_trb.dc = 0;
        cfg_trb.slot_id = device->get_slot();

        if (!_send_command_trb(reinterpret_cast<xhci_trb_t*>(&cfg_trb))) {
            log::error("[ xHCI ] Configure Endpoint failed for slot %u DCI %u",
                       device->get_slot(), dci);
            return -1;
        }

        return 0;
    }

    int32_t xhci_driver::_hid_set_idle(const xhci_device* device, const uint8_t interface_num) {
        xhci_device_request_packet req = {};
        req.bRequestType = 0x21;
        req.bRequest = 0x0A; // SET_IDLE
        req.wValue = 0; // report only on change
        req.wIndex = interface_num;
        req.wLength = 0;
        return _send_control_transfer(device, req, nullptr, 0);
    }

    int32_t xhci_driver::_hid_set_protocol(const xhci_device* device, const uint8_t interface_num, const uint8_t protocol) {
        xhci_device_request_packet req = {};
        req.bRequestType = 0x21;
        req.bRequest = 0x0B; // SET_PROTOCOL
        req.wValue = protocol; // 0=Boot, 1=Report
        req.wIndex = interface_num;
        req.wLength = 0;
        return _send_control_transfer(device, req, nullptr, 0);
    }
}
