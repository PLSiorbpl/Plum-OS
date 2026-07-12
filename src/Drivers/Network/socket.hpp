#pragma once
#include "std/types.hpp"
#include "std/Ring_Buffer.hpp"
#include "std/vector.hpp"

namespace soc {
    struct udp_recv_packet {
        uint32_t from_ip;
        uint16_t from_port;

        uint16_t size;
        uint8_t *data;
    };

    struct udp_send_packet {
        uint32_t to_ip;
        uint16_t to_port;

        uint16_t size;
        uint8_t *data;
    };

    struct Socket {
        int num = {};

        uint16_t local_port = {};

        uint32_t remote_ip = {};
        uint32_t remote_port = {};

        std::Ring_Buffer<udp_recv_packet, 32> rx_queue;
    };

    extern std::vector<Socket> sockets;

    Socket *find_socket(int soc);
    Socket *find_socket_port(uint16_t port);

    int socket();
    void bind(int sock, uint16_t port);
    void connect(int sock, uint32_t ip, uint16_t port);
    bool send(int sock, const void *data, size_t len);
    bool sendto(int sock, udp_send_packet *data);
    bool recv(int sock, udp_recv_packet &data, int timeout = 0);
    bool recvfrom(int sock, udp_recv_packet &data, uint32_t ip, uint16_t port, int timeout = 0);

}
