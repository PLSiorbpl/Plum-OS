#pragma once
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Net_Device.hpp"

namespace NET {
    void receive_IPv4(Net_Device *dev, const uint8_t *frame, uint16_t len);

    void send_IPv4(Net_Device* dev, uint32_t dst_ip, const uint8_t *frame, uint16_t d_size, uint8_t protocol);
}
