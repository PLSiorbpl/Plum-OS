#include "socket.hpp"

#include "kernel/log.h"
#include "kernel/Sleep.hpp"
#include "std/types.hpp"
#include "std/vector.hpp"
#include "std/printf.hpp"

namespace soc {
    std::vector<Socket> sockets;

    Socket *find_socket(const int soc) {
        for (auto &s : sockets) {
            if (s.num == soc)
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
    }

    void connect(const int sock, const uint32_t ip, const uint16_t port) {
        const auto s = find_socket(sock);
        if (s != nullptr) {
            s->remote_ip = ip;
            s->remote_port = port;
        }
    }

    bool send(int sock, const void *data, size_t len) {

    }

    bool sendto(int sock, const void *data, size_t len, uint32_t ip, uint16_t port) {

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

        std::printf("no socket: %i", std::Output::std_out, sock);
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

        std::printf("no socket: %i", std::Output::std_out, sock);
        return false;
    }
}
