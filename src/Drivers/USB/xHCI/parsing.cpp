#include "xHCI.hpp"
#include "std/types.hpp"
#include "kernel/log.h"
#include "structs.hpp"
#include "std/mem_common.hpp"

namespace USB {
    xhci_usb_config xhci_driver::_parse_config_descriptor(const uint8_t* buf, uint16_t total_length) {
        xhci_usb_config result = {};
        const uint8_t* ptr = buf;
        const uint8_t* end = buf + total_length;

        auto* cfg = reinterpret_cast<const usb_configuration_descriptor*>(ptr);
        result.config_value  = cfg->bConfigurationValue;
        result.attributes    = cfg->bmAttributes;
        result.max_power_ma  = cfg->bMaxPower * 2;

        ptr += cfg->bLength;
        xhci_usb_interface* current_iface = nullptr;

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

                    xhci_usb_interface new_iface = {};
                    new_iface.number            = iface->bInterfaceNumber;
                    new_iface.alternate_setting = iface->bAlternateSetting;
                    new_iface.class_code        = iface->bInterfaceClass;
                    new_iface.subclass_code     = iface->bInterfaceSubClass;
                    new_iface.protocol          = iface->bInterfaceProtocol;

                    result.interfaces.push_back(std::move(new_iface));

                    current_iface = &result.interfaces[result.interfaces.size() - 1];
                    break;
                }

                case USB_DESCRIPTOR_ENDPOINT: {
                    auto* ep = reinterpret_cast<const usb_endpoint_descriptor*>(ptr);

                    if (!current_iface) {
                        log::error("[ xHCI ] endpoint descriptor before any interface, skipping");
                        break;
                    }

                    xhci_usb_endpoint new_ep = {};
                    new_ep.address         = ep->bEndpointAddress;
                    new_ep.number          = USB_EP_NUM(ep->bEndpointAddress);
                    new_ep.is_in           = USB_EP_DIR_IN(ep->bEndpointAddress);
                    new_ep.type            = USB_EP_TYPE(ep->bmAttributes);
                    new_ep.max_packet_size = ep->wMaxPacketSize & 0x7FF;
                    new_ep.interval        = ep->bInterval;
                    new_ep.dci             = (new_ep.number * 2) + (new_ep.is_in ? 1 : 0);

                    current_iface->endpoints.push_back(new_ep);

                    static const char* ep_type_str[] = {"Control", "Isochronous", "Bulk", "Interrupt"};
                    break;
                }

                case USB_DESCRIPTOR_HID: {
                    auto* hid = reinterpret_cast<const usb_hid_descriptor*>(ptr);
                    current_iface->hid_descriptor = static_cast<usb_hid_descriptor>(*hid);
                    break;
                }

                default:
                    log::warn("[ xHCI ] (unknown descriptor type %x len=%u)", desc_type, desc_len);
                    break;
            }

            ptr += desc_len;
        }

        return result;
    }
}
