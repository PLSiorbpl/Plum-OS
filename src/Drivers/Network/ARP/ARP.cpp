#include "ARP.hpp"
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Ethernet.hpp"
#include "Drivers/Network/Net_Device.hpp"
#include "std/types.hpp"
#include <memory.hpp>

#include "kernel/log.h"

namespace NET {
    std::vector<ARPEntry> ARP_cache;

    void receive_ARP(Net_Device *dev, const uint8_t *frame, uint16_t len) {
        if (len < sizeof(ARPHeader)) {
            log::warn("[ NET ] ARP packet too short");
            return;
        }

        auto *arp = (ARPHeader *)(frame + sizeof(EthernetHeader));
        auto *eth = (EthernetHeader *)frame;

        if (arp->target_ip != Bswap_32(dev->get_ipv4())) return;

        switch (Bswap_16(arp->opcode)) {
            case ARP_REQUEST: {

                char buf[16];
                ipv4_to_str(arp->sender_ip, buf, true);
                //log::info("[ NET ] ARP from: %s", buf);

                ARPHeader reply{};

                reply.hardware_type = Bswap_16(1); // 1 - Ethernet

                reply.protocol_type = Bswap_16(IPv4_Ether_Type); // 0x0800

                reply.mac_len = 6;  // xx.xx.xx.xx.xx.xx MAC
                reply.ip_len = 4;   // xx.xx.xx.xx IPv4

                reply.opcode = Bswap_16(ARP_REPLY); // 2 - Reply

                memcpy(reply.sender_mac, dev->get_mac(), 6); // Me MAC
                reply.sender_ip = Bswap_32( dev->get_ipv4() ); // My ip

                memcpy(reply.target_mac, eth->src_mac, 6); // Sender MAC
                reply.target_ip = arp->sender_ip; // Sender IP

                send_ethernet(dev, reply.target_mac, Bswap_16(ARP_Ether_Type), &reply, sizeof(ARPHeader));
            }
            case ARP_REPLY: {
                ARPEntry entry = {};
                entry.ip = Bswap_32(arp->sender_ip);
                memcpy(entry.mac, arp->sender_mac, 6);

                for (auto &e : ARP_cache) {
                    if (e.ip == entry.ip) {
                        memcpy(e.mac, entry.mac, 6);
                        return;
                    }
                }
                ARP_cache.push_back(entry);
                return;
            }
            default: {

            }
        }
    }

    void send_ARP(Net_Device *dev, uint32_t target_ip) {
        char buf[16];
        ipv4_to_str(Bswap_32(target_ip), buf);
        //log::info("sending ARP to: %s", buf);
        ARPHeader request{};

        request.hardware_type = Bswap_16(1);
        request.protocol_type = Bswap_16(IPv4_Ether_Type);

        request.mac_len = 6;
        request.ip_len = 4;

        request.opcode = Bswap_16(ARP_REQUEST);

        memcpy(request.sender_mac, dev->get_mac(), 6);
        request.sender_ip = Bswap_32(dev->get_ipv4());

        memset(request.target_mac, 0, 6);
        request.target_ip = Bswap_32(target_ip);

        const uint8_t broadcast[6] = {
            0xff, 0xff, 0xff,
            0xff, 0xff, 0xff
        };

        send_ethernet(dev, broadcast, Bswap_16(ARP_Ether_Type), &request, sizeof(request));
    }

    uint8_t *arp_lookup(Net_Device *dev, const uint32_t ip) {
        for (auto &e : ARP_cache) {
            if (e.ip == ip)
                return e.mac;
        }
        send_ARP(dev, ip);

        for (int i = 0; i < 100; i++) {
            for (auto &e : ARP_cache) {
                if (e.ip == ip)
                    return e.mac;
            }
        }

        return nullptr;
    }
}
