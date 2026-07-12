#include "ARP.hpp"
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Ethernet.hpp"
#include "Drivers/Network/Net_Device.hpp"
#include "std/types.hpp"
#include "std/mem_common.hpp"
#include "kernel/log.h"

namespace NET {
    void receive_ARP(Net_Device *dev, const uint8_t *frame, uint16_t len) {
        if (len < sizeof(ARPHeader)) {
            log::warn("[ NET ] ARP packet too short");
            return;
        }

        auto *arp = (ARPHeader *)(frame + sizeof(EthernetHeader));

        auto *eth = (EthernetHeader *)frame;

        if (arp->target_ip != Bswap_32( dev->get_ipv4() )) return;
        if (arp->opcode == Bswap_16( ARP_REPLY )) return;

        char buf[16];
        ipv4_to_str(arp->sender_ip, buf, true);
        log::info("[ NET ] ARP from: %s", buf);

        ARPHeader reply{};

        reply.hardware_type = Bswap_16(1); // 1 - Ethernet

        reply.protocol_type = Bswap_16(IPv4_Ether_Type); // 0x0800

        reply.mac_len = 6;  // xx.xx.xx.xx.xx.xx MAC
        reply.ip_len = 4;   // xx.xx.xx.xx IPv4

        reply.opcode = Bswap_16(2); // 2 - Reply

        std::memcpy(reply.sender_mac, dev->get_mac(), 6); // Me MAC
        reply.sender_ip = Bswap_32( dev->get_ipv4() ); // My ip 10.0.0.2

        std::memcpy(reply.target_mac, eth->src_mac, 6);      // Sender MAC
        reply.target_ip = arp->sender_ip;                        // Sender IP

        send_ethernet(dev, reply.target_mac, Bswap_16(ARP_Ether_Type), &reply, sizeof(ARPHeader));
    }
}
