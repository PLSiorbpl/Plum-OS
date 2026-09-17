#pragma once
#include "std/types.hpp"

namespace NET {
    constexpr uint16_t ARP_Ether_Type = 0x0806;
    constexpr uint16_t IPv4_Ether_Type = 0x0800;
    constexpr uint16_t IPv4_Protocol_ICMP = 1;
    constexpr uint16_t IPv4_Protocol_UDP = 17;
    constexpr uint16_t IPv4_Protocol_TCP = 6;

    uint16_t Bswap_16(uint16_t x);

    uint32_t Bswap_32(uint32_t x);

    // 192, 168, 0, 1
    uint32_t make_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

    void ipv4_to_str(uint32_t ip, char *out, bool reverse = false);

    uint16_t checksum(const void* data, int len);
    uint16_t ipv4_checksum(uint32_t src_ip, uint32_t dst_ip, uint16_t protocol, const void* data, int len);

    struct EthernetHeader {
        uint8_t dst_mac[6];
        uint8_t src_mac[6];
        uint16_t ethertype;
    } __attribute__((packed));

    // ----------------------------------------------
    // ARP
    struct ARPHeader {
        uint16_t hardware_type; // Ethernet = 1
        uint16_t protocol_type; // IPv4 = 0x0800
        uint8_t mac_len; // 6
        uint8_t ip_len; // 4
        uint16_t opcode; // request=1, reply=2

        uint8_t sender_mac[6];
        uint32_t sender_ip;

        uint8_t target_mac[6];
        uint32_t target_ip;
    } __attribute__((packed));

    // ----------------------------------------------
    // IPv4
    struct IPv4Header {
        uint8_t  ihl_version; // Ipv4 version & lenght
        uint8_t  tos; // Type of service
        uint16_t total_length; // IP + UDP + data
        uint16_t id; // fragmentation
        uint16_t flags_frag;
        uint8_t  ttl; // time to live
        uint8_t  protocol; // ICMP UDP TCP IPv6
        uint16_t checksum;
        uint32_t src_ip;
        uint32_t dst_ip;
    } __attribute__((packed));

    struct ICMPHeader {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t id;
        uint16_t seq;
    } __attribute__((packed));

    struct UDPHeader {
        uint16_t src_port;
        uint16_t dst_port;
        uint16_t length;
        uint16_t checksum;
    } __attribute__((packed));

    struct ipv4_PseudoHeader {
        uint32_t src_ip;
        uint32_t dst_ip;

        uint8_t zero;
        uint8_t protocol;

        uint16_t len;
    } __attribute__((packed));

    struct TCP_Header {
        uint16_t src_port;
        uint16_t dst_port;

        uint32_t seq;
        uint32_t ack;

        uint8_t data_offset;
        uint8_t flags;

        uint16_t window;
        uint16_t checksum;
        uint16_t urgent;
    } __attribute__((packed));
    static_assert(sizeof(TCP_Header) == 20);

    enum class tcp_flags : uint8_t {
        FIN = 1,
        SYN = 2,
        RST = 4,
        PSH = 8,
        ACK = 16,
        URG = 32,

    };
}
