#include "Ethernet.hpp"

#include "Common.hpp"
#include "ARP/ARP.hpp"
#include "kernel/log.h"
#include "std/types.hpp"
#include <memory.hpp>
#include "IPv4/IPv4.hpp"
#include "kernel/Memory/heap.hpp"

namespace NET {
    void receive_ethernet(Net_Device *dev, const uint8_t *frame, const uint16_t len) {
        if (len < sizeof(EthernetHeader)) {
            log::warn("[ NET ] Packet too short");
            return;
        }

        auto eth = (EthernetHeader *)frame;

        switch (Bswap_16(eth->ethertype)) {
            case ARP_Ether_Type: {
                receive_ARP(dev, frame, len);
                return;
            }

            case IPv4_Ether_Type: {
                receive_IPv4(dev, frame, len);
                return;
            }

            default: {
                //log::info("[ NET ] Unknown: %x", static_cast<uint64_t>(Bswap_16(eth->ethertype)));

            }
        }
    }

    void send_ethernet(Net_Device *dev, const uint8_t *dst_mac, const uint16_t ethertype, const void *payload, uint16_t len) {
        if (len > 1500) {
            log::error("[ NET ] payload too big to send: %x", len);
            return;
        }
        if (payload == nullptr) {
            log::error("[ NET ] payload is nullptr");
            return;
        }

        auto *frame = static_cast<uint8_t *>(heap::malloc(sizeof(EthernetHeader) + len));
        auto* hdr = reinterpret_cast<EthernetHeader *>(frame);

        memcpy(hdr->dst_mac, dst_mac, 6);
        memcpy(hdr->src_mac, dev->get_mac(), 6);
        hdr->ethertype = ethertype;

        memcpy(frame + sizeof(EthernetHeader), payload, len);

        dev->send(frame, len + sizeof(EthernetHeader));
        heap::free(frame);
    }
}
