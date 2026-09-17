#pragma once
#include "Drivers/Network/Net_Device.hpp"
#include "std/types.hpp"

namespace tsock {
    class tcp_socket;
}

namespace NET {
    void receive_tcp(Net_Device *dev, const uint8_t *frame, uint16_t len);

    void send_tcp(Net_Device* dev, tsock::tcp_socket *socket, uint8_t flags, const uint8_t* payload, uint16_t payload_size);
}
