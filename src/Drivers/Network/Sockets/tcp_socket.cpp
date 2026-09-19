#include "tcp_socket.hpp"

#include "memory.hpp"
#include "Drivers/Network/Common.hpp"
#include "arch/x86_64/syscall/syscall.h"

namespace tsock {
    std::vector<std::unique_ptr<tcp_socket>> sockets;

    bool tcp_socket::bind(uint16_t port) {
        local_port = port;
        return true;
    }

    bool tcp_socket::listen() {
        if (state != tcp_state::CLOSED)
            return false;

        state = tcp_state::LISTEN;
        return true;
    }

    tcp_socket* tcp_socket::accept() {
        if (state != tcp_state::LISTEN || pending.empty())
            return nullptr;

        tcp_socket* client = pending.back();
        pending.pop_back();

        return client;
    }

    size_t tcp_socket::send(const void *data, size_t len) {
        tcp_syscall sys = {this, static_cast<u8>(NET::tcp_flags::ACK) | static_cast<u8>(NET::tcp_flags::PSH), (uint8_t *)data, len};
        const int ret = sys_tcp_socket(&sys, true, 0);
        send_seq += len;
        return ret;
    }

    size_t tcp_socket::recv(void *data, size_t len) {
        if (!data) return 0;
        if (rx_queue.empty()) return 0;

        cdata d;
        rx_queue.pop(d);
        if (len > d.size) len = d.size;

        memcpy(data, d.data, len);

        heap::free(d.data);
        return len;
    }

    void tcp_socket::close() {
        if (state == tcp_state::CLOSED)
            return;

        if (state == tcp_state::LISTEN) {
            state = tcp_state::CLOSED;
            for (auto &i : pending) {
                i->close();
            }
            remove_socket(num);
            return;
        }

        if (state == tcp_state::ESTABLISHED) {
            tcp_syscall sys = {this, static_cast<u8>(NET::tcp_flags::FIN) | static_cast<u8>(NET::tcp_flags::ACK), nullptr, 0};
            sys_tcp_socket(&sys, true, 0);

            send_seq++;
            state = tcp_state::FIN_WAIT_1;
            return;
        }

        remove_socket(num);
    }

    bool remove_socket(const int sock) {
        for (int i = 0; i < sockets.size(); i++) {
            if (sockets[i]->num == sock) {
                sockets.swap_remove(i);
                return true;
            }
        }
        return false;
    }

    tcp_socket *find_socket(const int sock) {
        for (auto &s : sockets) {
            if (s->num == sock)
                return s.get();
        }
        return nullptr;
    }

    tcp_socket *find_socket_listner(uint16_t local_port) {
        for (auto &s : sockets) {
            if (s->local_port == local_port && s->state == tcp_state::LISTEN)
                return s.get();
        }
        return nullptr;
    }

    tcp_socket *find_socket_connection(uint16_t local_port, uint32_t remote_ip, uint16_t remote_port) {
        for (auto &s : sockets) {
            if (s->local_port == local_port && s->remote_ip == remote_ip && s->remote_port == remote_port)
                return s.get();
        }
        return nullptr;
    }

    int socket() {
        for (int i = 0; i < 128; i++) {
            if (find_socket(i) == nullptr) {
                auto s = std::make_unique<tcp_socket>();
                s->num = i;
                sockets.push_back(std::move(s));
                return i;
            }
        }
        return -1;
    }
}
