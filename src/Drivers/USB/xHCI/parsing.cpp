#include "xHCI.hpp"
#include "std/types.hpp"
#include "kernel/log.h"
#include "structs.hpp"

namespace USB {
    void xhci_driver::_parse_config_descriptor(xhci_device* device, const uint8_t* buf, uint16_t total_length) {
        const uint8_t* ptr = buf;
        const uint8_t* end = buf + total_length;

        auto* cfg = reinterpret_cast<const usb_configuration_descriptor*>(ptr);
        log::info("[ xHCI ] config descriptor:");
        log::info("\tbNumInterfaces      = %u", cfg->bNumInterfaces);
        log::info("\tbConfigurationValue = %u", cfg->bConfigurationValue);
        log::info("\tbmAttributes        = %x", cfg->bmAttributes);
        log::info("\tbMaxPower           = %umA", cfg->bMaxPower * 2);

        ptr += cfg->bLength;

        while (ptr + 2 <= end) {
            uint8_t desc_len  = ptr[0];
            uint8_t desc_type = ptr[1];

            if (desc_len == 0) {
                log::error("[ xHCI ] zero-length descriptor, stopping parse");
                break;
            }
            if (ptr + desc_len > end) {
                log::error("[ xHCI ] descriptor overruns buffer, stopping parse");
                break;
            }

            switch (desc_type) {
                case USB_DESCRIPTOR_INTERFACE: {
                    auto* iface = reinterpret_cast<const usb_interface_descriptor*>(ptr);
                    log::info("Interface %u:", iface->bInterfaceNumber);
                    log::info("\tbInterfaceClass    = %x", iface->bInterfaceClass);
                    log::info("\tbInterfaceSubClass = %x", iface->bInterfaceSubClass);
                    log::info("\tbInterfaceProtocol = %x", iface->bInterfaceProtocol);
                    log::info("\tbNumEndpoints      = %u",   iface->bNumEndpoints);

                    if (iface->bInterfaceClass == USB_CLASS_HID) {
                        // HID protocol values: 1=Keyboard, 2=Mouse
                        const char* hid_type = (iface->bInterfaceProtocol == 1) ? "Keyboard"
                                             : (iface->bInterfaceProtocol == 2) ? "Mouse"
                                             : "HID";
                        log::success("\t-> %s (HID boot protocol)", hid_type);
                    }
                    break;
                }

                case USB_DESCRIPTOR_ENDPOINT: {
                    auto* ep = reinterpret_cast<const usb_endpoint_descriptor*>(ptr);
                    bool     is_in   = USB_EP_DIR_IN(ep->bEndpointAddress);
                    uint8_t  ep_num  = USB_EP_NUM(ep->bEndpointAddress);
                    uint8_t  ep_type = USB_EP_TYPE(ep->bmAttributes);

                    static const char* ep_type_str[] = { "Control", "Isochronous", "Bulk", "Interrupt" };

                    log::info("Endpoint %x:", ep->bEndpointAddress);
                    log::info("\tDirection      = %s", is_in ? "IN" : "OUT");
                    log::info("\tNumber         = %u", ep_num);
                    log::info("\tType           = %s", ep_type_str[ep_type & 3]);
                    log::info("\twMaxPacketSize = %u", ep->wMaxPacketSize);
                    log::info("\tbInterval      = %u", ep->bInterval);
                    break;
                }

                case USB_DESCRIPTOR_HID: {
                    auto* hid = reinterpret_cast<const usb_hid_descriptor*>(ptr);
                    log::info("HID descriptor:");
                    log::info("\tbcdHID              = %x", hid->bcdHID);
                    log::info("\tbNumDescriptors     = %u",   hid->bNumDescriptors);
                    log::info("\twClassDescriptorLen = %u",   hid->wClassDescriptorLength);
                    break;
                }

                default:
                    log::warn("(unknown descriptor type %x, len=%u)", desc_type, desc_len);
                    break;
            }

            ptr += desc_len;
        }
    }
}
