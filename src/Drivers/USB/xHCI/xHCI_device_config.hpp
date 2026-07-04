#pragma once
#include "structs.hpp"
#include "std/types.hpp"

namespace USB {
    struct xhci_usb_endpoint {
        uint8_t  address;        // raw bEndpointAddress (e.g. 0x81)
        uint8_t  number;         // address & 0x0F
        bool     is_in;          // address & 0x80
        uint8_t  type;           // bmAttributes & 0x03 (0=Control,1=Isoch,2=Bulk,3=Interrupt)
        uint16_t max_packet_size;
        uint8_t  interval;       // raw bInterval from descriptor (NOT yet adjusted for xHCI)
        uint8_t  dci;            // computed: (number*2) + (is_in ? 1 : 0)
    };

    struct xhci_usb_interface {
        uint8_t  number;
        uint8_t  alternate_setting;
        uint8_t  class_code;
        uint8_t  subclass_code;
        uint8_t  protocol;

        std::vector<xhci_usb_endpoint> endpoints;
        usb_hid_descriptor hid_descriptor;
    };

    struct xhci_usb_config {
        uint8_t  config_value;   // bConfigurationValue — pass to SET_CONFIGURATION
        uint8_t  attributes;
        uint8_t  max_power_ma;   // already multiplied by 2

        std::vector<xhci_usb_interface> interfaces;

        // Convenience: find first endpoint matching type+direction across all interfaces
        const xhci_usb_endpoint* find_endpoint(uint8_t type, bool is_in) const {
            for (size_t i = 0; i < interfaces.size(); i++) {
                const xhci_usb_interface& iface = interfaces[i];
                for (size_t j = 0; j < iface.endpoints.size(); j++) {
                    const xhci_usb_endpoint& ep = iface.endpoints[j];
                    if (ep.type == type && ep.is_in == is_in)
                        return &ep;
                }
            }
            return nullptr;
        }
    };
}
