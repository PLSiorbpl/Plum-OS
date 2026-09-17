#include "socket.hpp"

#include "../Drivers/RTL8139.hpp"
#include "kernel/Sleep.hpp"
#include "std/vector.hpp"
#include "std/printf.hpp"
#include "Drivers/Network/IPv4/UDP.hpp"
#include "kernel/log.h"

namespace soc {
    std::vector<Socket> sockets;

    Socket *find_socket(const int sock) {
        for (auto &s : sockets) {
            if (s.num == sock)
                return &s;
        }
        return nullptr;
    }

    Socket *find_socket_port(const uint16_t port) {
        for (auto &s : sockets) {
            if (s.local_port == port)
                return &s;
        }
        return nullptr;
    }

    int socket() {
        for (int i = 0; i < 64; i++) {
            if (find_socket(i) == nullptr) {
                Socket s = {};
                s.num = i;
                sockets.push_back(s);
                return i;
            }
        }
        return -1;
    }

    void bind(const int sock, const uint16_t port) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            s->local_port = port;
        }
        std::printf("[ soc ] No socket: %i", std::Output::std_out, sock);
    }

    void connect(const int sock, const uint32_t ip, const uint16_t port) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            s->remote_ip = ip;
            s->remote_port = port;
        }
        std::printf("[ soc ] No socket: %i", std::Output::std_out, sock);
    }

    bool send(const int sock, const udp_send_packet &data) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            NET::send_udp(&RTL8139::driver, s->remote_ip, s->remote_port, s->local_port, data.data, data.size);
            return true;
        }
        log::warn("[ soc ] No socket: %i", sock);
        return false;
    }

    bool sendto(const int sock, const udp_send_packet &data) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            NET::send_udp(&RTL8139::driver, data.to_ip, data.to_port, s->local_port, data.data, data.size);
            return true;
        }
        log::warn("[ soc ] No socket: %i", sock);
        return false;
    }

    bool recv(const int sock, udp_recv_packet &data, int timeout) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            while (s->rx_queue.empty()) {
                if (timeout <= 0) {
                    return false;
                }

                timeout -= 10;
                Time::Sleep(10);
            }

            udp_recv_packet src = {};
            s->rx_queue.pop(src);

            data = src;
            return true;
        }
        std::printf("[ soc ] No socket: %i", std::Output::std_out, sock);
        return false;
    }

    bool recvfrom(const int sock, udp_recv_packet &data, const uint32_t ip, const uint16_t port, int timeout) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            while (s->rx_queue.empty()) {
                if (timeout <= 0) {
                    return false;
                }

                timeout -= 10;
                Time::Sleep(10);
            }

            udp_recv_packet src = {};
            s->rx_queue.pop(src);
            if (src.from_ip == ip && src.from_port == port) {
                data = src;
                return true;
            }
            return false;
        }

        std::printf("[ soc ] No socket: %i", std::Output::std_out, sock);
        return false;
    }
}
