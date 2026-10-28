#pragma once
#include "ahci_port.h"
#include "arch/x86_64/IDT/IDT.hpp"
#include "std/types.hpp"

namespace drivers::ahci {
    class ahci_device;
    constexpr u64 AHCI_ENABLE_BIT = (1 << 31);

    struct hba_memory {
        volatile struct capabilities {
            u32 num_ports : 5;
            u32 supports_external_sata : 1;
            u32 enclosure_management_supported : 1;
            u32 command_completion_coalescing_supported : 1;
            u32 num_command_slots : 5;
            u32 partial_state_capable : 1;
            u32 slumber_state_capable : 1;
            u32 pio_multiple_drq_block : 1;
            u32 fis_based_switching_supported : 1;
            u32 port_multiplier_supported : 1;
            u32 ahci_mode_only : 1;
            u32 reserved : 1;
            u32 interface_speed_support : 4;
            u32 supports_command_list_override : 1;
            u32 supports_activity_led : 1;
            u32 supports_aggressive_link_power_management : 1;
            u32 supports_staggered_spin_up : 1;
            u32 supports_mechanical_presence_switch : 1;
            u32 supports_snotification_register : 1;
            u32 supports_native_command_queue : 1;
            u32 supports_64_bit_addressing : 1;
        } capabilities;

        volatile struct ghc_reg {
            u32 reset_hba : 1;
            u32 interrupts_enabled : 1;
            u32 msi_revert_to_single_msg : 1;
            u32 reserved : 28;
            u32 ahci_enable : 1;
        } ghc;

        volatile u32 is;	// 0x08, Interrupt status
        volatile u32 pi;	// 0x0C, Port implemented
        volatile u32 vs;	// 0x10, Version
        volatile u32 ccc_ctl; // 0x14, Command completion coalescing control
        volatile u32 ccc_pts; // 0x18, Command completion coalescing ports
        volatile u32 em_loc;	// 0x1C, Enclosure management location
        volatile u32 em_ctl;	// 0x20, Enclosure management control
        volatile u32 cap2; // 0x24, Host capabilities extended
        volatile u32 bohc; // 0x28, BIOS/OS handoff control and status

        // 0x2C - 0x9F, Reserved
        volatile u8 rsv[0xA0-0x2C];

        // 0xA0 - 0xFF, Vendor specific registers
        volatile u8 vendor[0x100-0xA0];

        // 0x100 - 0x10FF, Port control registers
        volatile hba_port ports[32];	// 1 ~ 32
    };

    class ahci {
    public:
        ahci();
        ~ahci();

        void init();

        ahci_device *request_device(u32 id);
    private:
        void on_interrupt(const IDT::ISR_Registers* isr);
        void probe_ports();

        ahci_port ports[32] {};
        u8 port_count = 0;
        volatile hba_memory* hba = nullptr;
    };
}
