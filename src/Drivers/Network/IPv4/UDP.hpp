#pragma once
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Net_Device.hpp"

namespace NET {
    void receive_udp(Net_Device *dev, const uint8_t *frame, uint16_t len);

    void send_udp(Net_Device* dev, uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, const uint8_t* payload, uint16_t payload_size);
}
