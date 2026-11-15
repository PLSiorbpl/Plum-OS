#pragma once
#include "libs/std/types.hpp"
#include "arch/x86_64/IDT/IDT.hpp"

namespace PCI {
    uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
    uint16_t pci_read16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
    uint8_t pci_read8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

    void pci_write32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, u32 value);
    void pci_write16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, u16 value);
    void pci_write8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, u8 value);

    struct PCI_Device {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
        uint16_t vendor_id;
        uint16_t device_id;
        uint32_t bar[6];

        uint32_t read32(const uint8_t offset) const { return pci_read32(bus, device, function, offset); }
        uint16_t read16(const uint8_t offset) const { return pci_read16(bus, device, function, offset); }
        uint8_t read8(const uint8_t offset) const { return pci_read8(bus, device, function, offset); }

        void write32(const uint8_t offset, const uint32_t value) const { pci_write32(bus, device, function, offset, value); }
        void write16(const uint8_t offset, const uint16_t value) const { pci_write16(bus, device, function, offset, value); }
        void write8(const uint8_t offset, const uint8_t value) const { pci_write8(bus, device, function, offset, value); }
    };

    uint64_t get_msi_offset(PCI_Device device);
    void install_interrupt(const PCI_Device &dev, IDT::isr_t handler, uint8_t irq = 255);

    void Test();

    uint32_t get_class(uint8_t bus, uint8_t dev, uint8_t fn);

    PCI_Device Find(uint32_t vendor, uint32_t device_);
    PCI_Device Find_Class(uint32_t Class);
    PCI_Device find_vendor_class(uint32_t vendor, uint8_t base_class, uint8_t sub_class);
    PCI_Device find_class_with_sub(u8 base_class, u8 sub_class);
}
