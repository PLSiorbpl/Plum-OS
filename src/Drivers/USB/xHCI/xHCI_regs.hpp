#pragma once
#include "std/types.hpp"

namespace USB {
    struct xhci_capability_registers {
        const uint8_t caplength;    // Capability Register Length
        const uint8_t reserved0;
        const uint16_t hciversion;  // Interface Version Number
        const uint32_t hcsparams1;  // Structural Parameters 1
        const uint32_t hcsparams2;  // Structural Parameters 2
        const uint32_t hcsparams3;  // Structural Parameters 3
        const uint32_t hccparams1;  // Capability Parameters 1
        const uint32_t dboff;       // Doorbell Offset
        const uint32_t rtsoff;      // Runtime Register Space Offset
        const uint32_t hccparams2;  // Capability Parameters 2
    };
    static_assert(sizeof(xhci_capability_registers) == 32);

    struct xhci_operational_registers {
        uint32_t usbcmd;        // USB Command
        uint32_t usbsts;        // USB Status
        uint32_t pagesize;      // Page Size
        uint32_t reserved0[2];
        uint32_t dnctrl;        // Device Notification Control
        uint64_t crcr;          // Command Ring Control
        uint32_t reserved1[4];
        uint64_t dcbaap;        // Device Context Base Address Array Pointer
        uint32_t config;        // Configure
        uint32_t reserved2[49];
        // Port Register Set offset has to be calculated dynamically based on MAXPORTS
    };
    static_assert(sizeof(xhci_operational_registers) == 256);

    struct xhci_interrupter_registers {
        uint32_t iman;
        uint32_t imod;
        uint32_t erstsz;
        uint32_t rsvd;
        uint64_t erstba;
        union {
            struct {
                uint64_t dequeue_erst_segment_index : 3;
                uint64_t event_handler_busy : 1;
                uint64_t event_ring_dequeue_pointer : 60;
            };
            uint64_t erdp;
        };
    };

    struct xhci_runtime_registers {
        uint32_t mf_index;
        uint32_t rsvdz[7];
        xhci_interrupter_registers ir[1024];
    };

    struct xhci_doorbell_register {
    union {
        struct {
            uint8_t db_target;
            uint8_t rsvd;
            uint16_t db_stream_id;
        };
        uint32_t raw;
    };
    };

    class xhci_doorbell_manager {
    public:
        xhci_doorbell_manager(uint64_t base);

        void ring_doorbell(uint8_t doorbell, uint8_t target) const;

        void ring_command_doorbell() const;
        void ring_control_endpoint_doorbell(uint8_t doorbell) const;

    private:
        xhci_doorbell_register *m_doorbell_registers;

    };

    struct xhci_extended_capability_entry {
        union {
            struct {
                uint8_t id;
                uint8_t next;
                uint16_t cap_specific;
            };
            uint32_t raw;
        };
    };
    static_assert(sizeof(xhci_extended_capability_entry) == 4);

    #define XHCI_NEXT_EXT_CAP_PTR(ptr, next) (volatile uint32_t*)((char*)ptr + (next * sizeof(uint32_t)))

    enum class xhci_extended_capability_code {
        reserved                            = 0,
        usb_legacy_support                  = 1,
        supported_protocol                  = 2,
        extended_power_management           = 3,
        iovirtualization_support            = 4,
        message_interrupt_support           = 5,
        local_memory_support                = 6,
        usb_debug_capability_support        = 10,
        extended_message_interrupt_support  = 17
    };

    class xhci_extended_capability {
    public:
        xhci_extended_capability(volatile uint32_t* cap_ptr);

        [[nodiscard]] volatile uint32_t* base() const { return m_base; }

        [[nodiscard]] xhci_extended_capability_code id() const {
            return static_cast<xhci_extended_capability_code>(m_entry.id);
        }

        [[nodiscard]] xhci_extended_capability *next() const { return m_next; }

    private:
        volatile uint32_t* m_base = nullptr;
        xhci_extended_capability_entry m_entry = {};
        xhci_extended_capability *m_next = {};

    private:
        void _read_next_ext_caps();
    };

    struct xhci_portsc_register {
        union {
            struct {
                // Current connect status (RO), if PP is 0, this bit is also 0
                uint32_t    ccs         : 1;

                // Port Enable/Disable (R/WC), if PP is 0, this bit is also 0
                uint32_t    ped         : 1;

                // Reserved and zeroed
                uint32_t    rsvd0       : 1;

                // Over-current active (RO)
                uint32_t    oca         : 1;

                // Port reset (R/W), if PP is 0, this bit is also 0
                uint32_t    pr          : 1;

                // Port link state (R/W), if PP is 0, this bit is also 0
                uint32_t    pls         : 4;

                // Port power (R/W)
                uint32_t    pp          : 1;

                // Port speed (RO)
                uint32_t    port_speed  : 4;

                // Port indicator control (R/W), if PP is 0, this bit is also 0
                uint32_t    pic         : 2;

                // Port link state write strobe (R/W), if PP is 0, this bit is also 0
                uint32_t    lws         : 1;

                // Connect status change (R/WC), if PP is 0, this bit is also 0.
                // ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                // ** Clear this bit by writing a '1' to it.
                uint32_t    csc         : 1;

                /*
                Port enable/disable change (R/WC), if PP is 0, this bit is also 0.
                ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                ** For a USB2 protocol port, this bit shall be set to ‘1’ only when the port is disabled (EOF2)
                ** For a USB3 protocol port, this bit shall never be set to ‘1’
                ** Software shall clear this bit by writing a ‘1’ to it. Refer to section 4.19.2
                */
                uint32_t    pec         : 1;

                // Warm port reset change (R/WC), if PP is 0, this bit is also 0.
                // ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                // ** Reserved and zeroed on USB2 ports.
                // ** Software shall clear this bit by writing a '1' to it.
                uint32_t    wrc         : 1;

                // Over-current change (R/WC), if PP is 0, this bit is also 0.
                // ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                // ** Software shall clear this bit by writing a '1' to it.
                uint32_t    occ         : 1;

                // Port reset change (R/WC), if PP is 0, this bit is also 0.
                // ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                // ** Software shall clear this bit by writing a '1' to it.
                uint32_t    prc         : 1;

                // Port link state change (R/WC), if PP is 0, this bit is also 0.
                // ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                uint32_t    plc         : 1;

                // Port config error change (R/WC), if PP is 0, this bit is also 0.
                // ** When transitioning from 0 to a 1, will trigger a Port Status Change Event.
                // ** Reserved and zeroed on USB2 ports.
                // ** Software shall clear this bit by writing a '1' to it.
                uint32_t    cec         : 1;

                // Cold attach status (R/O), if PP is 0, this bit is also 0.
                uint32_t    cas         : 1;

                // Wake on connect enable (R/W)
                uint32_t    wce         : 1;

                // Wake on disconnect enable (R/W)
                uint32_t    wde         : 1;

                // Wake on over-current enable (R/W)
                uint32_t    woe         : 1;

                // Reserved and zeroed
                uint32_t    rsvd1        : 2;

                // Device removable (RO)
                uint32_t    dr          : 1;

                // Warm port reset (R/WC).
                // ** Reserved and zeroed on USB2 ports.
                uint32_t    wpr         : 1;
            } __attribute__((packed));

            // Must be accessed using 32-bit dwords
            uint32_t raw;
        };
    } __attribute__((packed));
    static_assert(sizeof(xhci_portsc_register) == sizeof(uint32_t));
}