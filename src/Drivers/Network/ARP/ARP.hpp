#pragma once
#include "Drivers/Network/Net_Device.hpp"
#include "std/types.hpp"
#include "std/vector.hpp"

namespace NET {
    constexpr uint16_t ARP_REQUEST = 1;
    constexpr uint16_t ARP_REPLY   = 2;

    struct ARPEntry {
        uint32_t ip;
        uint8_t mac[6];
    };

    extern std::vector<ARPEntry> ARP_cache;

    void receive_ARP(Net_Device *dev, const uint8_t *frame, uint16_t len);
    void send_ARP(Net_Device *dev, uint32_t target_ip);

    uint8_t *arp_lookup(Net_Device *dev, uint32_t ip);
}
