#include "IPv4.hpp"

#include "UDP.hpp"
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Ethernet.hpp"
#include "Drivers/Network/Net_Device.hpp"
#include "Drivers/Network/IPv4/ICMP.hpp"
#include "TCP.hpp"
#include "Drivers/Network/ARP/ARP.hpp"
#include "kernel/log.h"

namespace NET {
    void receive_IPv4(Net_Device *dev, const uint8_t *frame, const uint16_t len) {
        if (len < sizeof(IPv4Header) + sizeof(EthernetHeader)) {
            log::warn("IPv4 packet too short");
            return;
        }

        const auto ip = (IPv4Header *)(frame + sizeof(EthernetHeader));
        if (ip->dst_ip != Bswap_32( dev->get_ipv4() )) return; // not to us

        switch (ip->protocol) {
            case IPv4_Protocol_ICMP: {
                receive_ICMP(dev, frame, len);
                return;
            }
            case IPv4_Protocol_UDP: {
                receive_udp(dev, frame, len);
                return;
            }
            case IPv4_Protocol_TCP: {
                receive_tcp(dev, frame, len);
                return;
            }
            default:
                //log::info("[ IPv4 ] unknown: %u", ip->protocol);
                return;
        }
    }

    void send_IPv4(Net_Device* dev, uint32_t dst_ip, const uint8_t *frame, uint16_t d_size, uint8_t protocol) {
        const uint16_t ip_size = sizeof(IPv4Header) + d_size;

        auto* ip = (IPv4Header*)frame;

        // -------------------------------------
        // IPv4
        ip->ihl_version = (4 << 4) | 5;
        ip->tos = 0;

        ip->total_length = Bswap_16(ip_size);

        ip->id = 0;
        ip->flags_frag = 0;

        ip->ttl = 64;
        ip->protocol = protocol;

        ip->src_ip = Bswap_32(dev->get_ipv4());
        ip->dst_ip = Bswap_32(dst_ip);

        ip->checksum = 0;
        ip->checksum = checksum(ip, sizeof(IPv4Header));

        // -------------------------------------
        // Ethernet
        const uint8_t *dst_mac = arp_lookup(dev, Bswap_32(ip->dst_ip));
        if (!dst_mac) {
            char buf[16];
            ipv4_to_str(Bswap_32(ip->dst_ip), buf);
            log::info("Cant find that ip: %s", buf);
            return;
        }

        send_ethernet(dev, dst_mac, Bswap_16(IPv4_Ether_Type), ip, ip_size);
    }
}
