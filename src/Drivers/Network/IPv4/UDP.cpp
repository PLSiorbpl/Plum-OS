#include "UDP.hpp"
#include "Drivers/Network/Common.hpp"
#include "kernel/Memory/heap.hpp"
#include "std/mem_common.hpp"
#include "kernel/log.h"
#include "Drivers/Network/socket.hpp"

namespace NET {
    void receive_udp(Net_Device *dev, const uint8_t *frame, uint16_t len) {
        const auto *ip = (IPv4Header *)(frame + sizeof(EthernetHeader));
        const uint8_t ip_header_len = (ip->ihl_version & 0x0F) * 4;

        auto* udp = (UDPHeader *)(frame + sizeof(EthernetHeader) + ip_header_len);

        // TODO
        // Implement checksum checking

        const uint16_t payload_len = Bswap_16(udp->length) - sizeof(UDPHeader);

        auto *buf = static_cast<uint8_t *>(heap::malloc(payload_len));
        const uint8_t* payload = reinterpret_cast<uint8_t *>(udp) + sizeof(UDPHeader);
        std::memcpy(buf, payload, payload_len);

        const auto socket = soc::find_socket_port(Bswap_16(udp->dst_port));
        if (socket != nullptr) {
            soc::udp_recv_packet recv = {};
            recv.from_port = Bswap_16(udp->src_port);
            recv.from_ip = Bswap_32(ip->src_ip);
            recv.size = payload_len;
            recv.data = buf;
            socket->rx_queue.push(recv);
        }
    }

    //void send_udp(Net_Device *dev, )
}
