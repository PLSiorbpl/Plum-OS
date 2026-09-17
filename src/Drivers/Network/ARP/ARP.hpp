#pragma once
#include "Drivers/Network/Net_Device.hpp"
#include "std/types.hpp"
#include "std/vector.hpp"

namespace NET {
    constexpr uint16_t ARP_REQUEST = 1;
    constexpr uint16_t ARP_REPLY   = 2;

    struct ARP_c {
        uint8_t mac[6];
        uint32_t ip;
    };

    extern std::vector<ARP_c> ARP_cache;

    void receive_ARP(Net_Device *dev, const uint8_t *frame, uint16_t len);
}
