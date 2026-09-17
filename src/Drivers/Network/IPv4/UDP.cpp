#include "UDP.hpp"
#include "Drivers/Network/Common.hpp"
#include "kernel/Memory/heap.hpp"
#include <memory.hpp>

#include "IPv4.hpp"
#include "../Sockets/socket.hpp"

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
        memcpy(buf, payload, payload_len);

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

    void send_udp(Net_Device* dev, const uint32_t dst_ip, const uint16_t dst_port, const uint16_t src_port, const uint8_t* payload, const uint16_t payload_size) {
        const uint16_t udp_size = sizeof(UDPHeader) + payload_size;
        const uint16_t ip_size = sizeof(IPv4Header) + udp_size;

        auto* ip = static_cast<uint8_t *>(heap::malloc(ip_size));
        auto* udp = reinterpret_cast<UDPHeader *>(ip + sizeof(IPv4Header));

        // -------------------------------------
        // UDP
        udp->src_port = Bswap_16(src_port);
        udp->dst_port = Bswap_16(dst_port);
        udp->length = Bswap_16(udp_size);
        udp->checksum = 0;
        memcpy(reinterpret_cast<uint8_t *>(udp) + sizeof(UDPHeader), payload, payload_size);

        // -------------------------------------
        // UDP checksum
        udp->checksum = ipv4_checksum(dev->get_ipv4(), dst_ip, IPv4_Protocol_UDP, udp, udp_size);

        // -------------------------------------
        // IPv4
        send_IPv4(dev, dst_ip, ip, udp_size, IPv4_Protocol_UDP);
        heap::free(ip);
    }
}
