#pragma once
#include "std/types.hpp"

namespace USB {
#define USB_DESCRIPTOR_CONFIGURATION  0x02
#define USB_DESCRIPTOR_INTERFACE      0x04
#define USB_DESCRIPTOR_ENDPOINT       0x05
#define USB_DESCRIPTOR_HID            0x21

#define USB_CLASS_HID                 0x03

#define USB_EP_DIR_IN(addr)           (((addr) & 0x80) != 0)
#define USB_EP_NUM(addr)              ((addr) & 0x0F)
#define USB_EP_TYPE(attr)             ((attr) & 0x03)
#define USB_EP_TYPE_INTERRUPT         0x03

    struct usb_descriptor_header {
        uint8_t bLength;
        uint8_t bDescriptorType;
    } __attribute__((packed));
    static_assert(sizeof(usb_descriptor_header) == 2);

    struct xhci_evaluate_context_command_trb_t {
        uint64_t input_context_physical_base;
        uint32_t rsvd        : 22;
        uint32_t rsvd1       : 9;
        uint32_t cycle_bit   : 1;
        uint32_t trb_type    : 6;
        uint32_t rsvd2       : 16;
        uint32_t slot_id     : 8;
    } __attribute__((packed));

    struct usb_device_descriptor {
        usb_descriptor_header header;
        uint16_t bcdUsb;
        uint8_t bDeviceClass;
        uint8_t bDeviceSubClass;
        uint8_t bDeviceProtocol;
        uint8_t bMaxPacketSize0;
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t iManufacturer;
        uint8_t iProduct;
        uint8_t iSerialNumber;
        uint8_t bNumConfigurations;
    } __attribute__((packed));
    static_assert(sizeof(usb_device_descriptor) == 18);

    struct usb_configuration_descriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;   // 0x02
        uint16_t wTotalLength;      // total bytes including all trailing descriptors
        uint8_t  bNumInterfaces;
        uint8_t  bConfigurationValue;
        uint8_t  iConfiguration;
        uint8_t  bmAttributes;
        uint8_t  bMaxPower;         // in 2mA units
    } __attribute__((packed));

    struct usb_interface_descriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;   // 0x04
        uint8_t  bInterfaceNumber;
        uint8_t  bAlternateSetting;
        uint8_t  bNumEndpoints;
        uint8_t  bInterfaceClass;
        uint8_t  bInterfaceSubClass;
        uint8_t  bInterfaceProtocol;
        uint8_t  iInterface;
    } __attribute__((packed));

    struct usb_endpoint_descriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;   // 0x05
        uint8_t  bEndpointAddress;  // bit7=direction(1=IN), bits3:0=endpoint number
        uint8_t  bmAttributes;      // bits1:0: 0=Control,1=Isoch,2=Bulk,3=Interrupt
        uint16_t wMaxPacketSize;
        uint8_t  bInterval;         // polling interval in frames/microframes
    } __attribute__((packed));

    struct usb_hid_descriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;   // 0x21
        uint16_t bcdHID;
        uint8_t  bCountryCode;
        uint8_t  bNumDescriptors;
        uint8_t  bClassDescriptorType;
        uint16_t wClassDescriptorLength;
    } __attribute__((packed));
}
