#include "ahci.h"

#include "ahci_device.h"
#include "ahci_helper.h"
#include "../PCI.hpp"
#include "../../kernel/Paging.hpp"
#include "../../libs/std/printf.hpp"
#include "arch/x86_64/Common/Common.hpp"
#include "kernel/log.h"
#include "kernel/Sleep.hpp"
#include "kernel/system.hpp"

drivers::ahci::ahci::ahci() {

}

drivers::ahci::ahci::~ahci() {

}

void drivers::ahci::ahci::init() {
    if (hba != nullptr) {
        log::warn("ACHI was already initialized.");
        return;
    }

    const auto device = PCI::find_class_with_sub(0x01, 0x06);
    //const u8 irq = PCI::pci_read8(device.bus, device.device, device.function, 0x3C);

    constexpr uint8_t irq = 11;
    PCI::install_interrupt(device, [](const IDT::ISR_Registers* regs) {
        const auto ahci = &systemPL::ahci;
        ahci->on_interrupt(regs);
    }, irq);

    hba = reinterpret_cast<volatile hba_memory*>(static_cast<u64>(device.bar[5] & 0xFFFFFFF0));
    Paging::Map_memory(reinterpret_cast<u64>(hba), reinterpret_cast<u64>(hba) + sizeof(hba_memory), Paging::Profile::MMIO);


    hba->ghc.ahci_enable = true;
    while (!hba->ghc.ahci_enable) {}
    probe_ports();
    hba->ghc.interrupts_enabled = true;
    while (!hba->ghc.interrupts_enabled) {}

    Time::Sleep(100);
}

drivers::ahci::ahci_device *drivers::ahci::ahci::request_device(const u32 id) {
    if (id >= 32) {
        log::warn("Attempted to request non existant device!");
        return new ahci_device(nullptr, false);
    }
    return new ahci_device(&ports[id], ports[id].is_active());
}

void drivers::ahci::ahci::on_interrupt(const IDT::ISR_Registers* isr) {
    const u32 global_is = hba->is;
    if (!global_is)
        return;
    hba->is = global_is;

    for (int i = 0; i < 32; ++i) {
        if (global_is & (1 << i)) { // port interrupted
            ports[i].on_interrupt();
        }
    }
}

void drivers::ahci::ahci::probe_ports() {
    const u32 ports_implemented = hba->pi;
    for (int i = 0; i < 32; ++i) {
        if (ports_implemented & (1 << i)) {
            auto port_type = get_port_type(&hba->ports[i]);
            if (port_type == port_type::sata || port_type == port_type::satapi)
                ports[i].configure(port_type, &hba->ports[i], i, hba);
        }
    }
}
