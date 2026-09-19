#include "TCP.hpp"

#include "IPv4.hpp"
#include "memory.hpp"
#include "kernel/log.h"
#include "Drivers/Network/Common.hpp"
#include "Drivers/Network/Sockets/tcp_socket.hpp"
#include "kernel/Memory/heap.hpp"

namespace NET {
    void receive_tcp(Net_Device *dev, const uint8_t *frame, uint16_t len) {
        const auto *ip = (IPv4Header *)(frame + sizeof(EthernetHeader));
        const uint8_t ip_header_len = (ip->ihl_version & 0x0F) * 4;

        auto* tcp = (TCP_Header *)(frame + sizeof(EthernetHeader) + ip_header_len);

        const uint16_t src_port = Bswap_16(tcp->src_port);
        const uint16_t dst_port = Bswap_16(tcp->dst_port);
        const uint32_t seq = Bswap_32(tcp->seq);
        const uint32_t ack = Bswap_32(tcp->ack);
        const uint8_t tcp_header_len = (tcp->data_offset >> 4) * 4;
        const uint8_t *payload = reinterpret_cast<uint8_t *>(tcp) + tcp_header_len;
        const uint32_t payload_size = Bswap_16(ip->total_length) - ip_header_len - tcp_header_len;

        if (auto s = tsock::find_socket_connection(dst_port, Bswap_32(ip->src_ip), src_port)) {
            if (payload_size > 0 && payload_size < 8197) {
                tsock::cdata d = {};
                d.data = heap::malloc(payload_size);
                d.size = payload_size;
                memcpy(d.data, payload, payload_size);
                s->rx_queue.push(d);
                s->recv_seq += payload_size;
                send_tcp(dev, s, (u8)tcp_flags::ACK, nullptr, 0);
                return;
            }

            if (tcp->flags & (uint8_t)tcp_flags::FIN) {
                s->recv_seq++;

                send_tcp(dev, s, (u8)tcp_flags::ACK, nullptr, 0);

                if (s->state == tsock::tcp_state::FIN_WAIT_2) {
                    s->state = tsock::tcp_state::TIME_WAIT;
                }
                else if (s->state == tsock::tcp_state::FIN_WAIT_1) {
                    if ((tcp->flags & (u8)tcp_flags::ACK) &&
                        ack == s->send_seq) {
                        s->state = tsock::tcp_state::TIME_WAIT;
                        } else {
                            s->state = tsock::tcp_state::CLOSING;
                        }
                }
                else if (s->state == tsock::tcp_state::ESTABLISHED) {
                    s->state = tsock::tcp_state::CLOSE_WAIT;
                }
            }

            if (tcp->flags & (uint8_t)tcp_flags::ACK) {
                if (s->state == tsock::tcp_state::FIN_WAIT_1)
                    s->state = tsock::tcp_state::FIN_WAIT_2;
            }

            if (tcp->flags & (u8)tcp_flags::SYN) {
                if (s->state == tsock::tcp_state::SYN_RECEIVED) {
                    //log::info("SYN retransmit %u", tsock::sockets.size());
                    s->recv_seq = seq + 1;
                    s->send_seq = 1000;
                    send_tcp(dev, s, (u8)tcp_flags::SYN | (u8)tcp_flags::ACK, nullptr, 0); // ACK + SYN
                    s->send_seq++;
                    return;
                }
            }

            if (tcp->flags & (uint8_t)tcp_flags::ACK) {
                if (s->state == tsock::tcp_state::SYN_RECEIVED && ack == s->send_seq) {
                    s->state = tsock::tcp_state::ESTABLISHED;

                    auto listener = tsock::find_socket_listner(dst_port);
                    if (listener)
                        listener->pending.push_back(s);
                    //log::info("Yes it works ");
                }
            }
            return;
        }

        if (tcp->flags & static_cast<uint8_t>(tcp_flags::SYN)) {
            auto sock = tsock::find_socket_listner(dst_port);

            if (sock && sock->state == tsock::tcp_state::LISTEN) {
                //log::info("New SYN %u", tsock::sockets.size());
                int ss = tsock::socket();
                auto s = tsock::find_socket(ss);
                s->local_port = dst_port;
                s->remote_port = src_port;
                s->remote_ip = Bswap_32(ip->src_ip);
                s->recv_seq = seq + 1;
                s->send_seq = 1000;
                s->state = tsock::tcp_state::SYN_RECEIVED;
                send_tcp(dev, s, (u8)tcp_flags::SYN | (u8)tcp_flags::ACK, nullptr, 0); // ACK + SYN
                s->send_seq++;
                return;
            }
        }
    }

    void send_tcp(Net_Device *dev, tsock::tcp_socket *socket, uint8_t flags, const uint8_t *payload, uint16_t payload_size) {
        const uint16_t tcp_size = sizeof(TCP_Header) + payload_size;
        const uint16_t ip_size = sizeof(IPv4Header) + tcp_size;

        auto* ip = static_cast<uint8_t *>(heap::malloc(ip_size));
        auto* tcp = reinterpret_cast<TCP_Header *>(ip + sizeof(IPv4Header));

        tcp->dst_port = Bswap_16(socket->remote_port);
        tcp->src_port = Bswap_16(socket->local_port);
        tcp->seq = Bswap_32(socket->send_seq);
        tcp->ack = Bswap_32(socket->recv_seq);
        tcp->data_offset = 5 << 4;
        tcp->flags = flags;
        tcp->checksum = 0;
        tcp->window = Bswap_16(65535);
        tcp->urgent = 0;
        if (payload)
            memcpy(reinterpret_cast<uint8_t *>(tcp) + sizeof(TCP_Header), payload, payload_size);

        // -------------------------------------
        // TCP checksum
        tcp->checksum = ipv4_checksum(dev->get_ipv4(), socket->remote_ip, IPv4_Protocol_TCP, tcp, tcp_size);

        send_IPv4(dev, socket->remote_ip, ip, tcp_size, IPv4_Protocol_TCP);
        heap::free(ip);
    }
}
