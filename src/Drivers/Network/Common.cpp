#include "Common.hpp"

#include "memory.hpp"
#include "kernel/Memory/heap.hpp"
#include "std/types.hpp"

namespace NET {
    uint16_t Bswap_16(const uint16_t x) {
        return (x << 8) | (x >> 8);
    }

    uint32_t Bswap_32(const uint32_t x) {
        return ((x << 24) & 0xFF000000) |
               ((x << 8)  & 0x00FF0000) |
               ((x >> 8)  & 0x0000FF00) |
               ((x >> 24) & 0x000000FF);
    }

    uint32_t make_ipv4(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) {
        return (static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(b) << 16) |
            (static_cast<uint32_t>(c) << 8 ) |
            (static_cast<uint32_t>(d));
    }

    void ipv4_to_str(uint32_t ip, char *out, const bool reverse) {
        if (reverse) {
            ip = Bswap_32(ip);
        }
        const uint8_t a = (ip >> 24) & 0xFF;
        const uint8_t b = (ip >> 16) & 0xFF;
        const uint8_t c = (ip >> 8) & 0xFF;
        const uint8_t d = (ip) & 0xFF;

        char* p = out;

        auto write_num = [&](uint8_t v) {
            if (v >= 100) {
                *p++ = '0' + (v / 100);
                v %= 100;
            }
            if (v >= 10) {
                *p++ = '0' + (v / 10);
                v %= 10;
            }
            *p++ = '0' + v;
        };

        write_num(a); *p++ = '.';
        write_num(b); *p++ = '.';
        write_num(c); *p++ = '.';
        write_num(d); *p++ = '\0';
    }

    uint16_t checksum(const void* data, int len) {
        const auto* ptr = static_cast<const uint16_t *>(data);

        uint32_t sum = 0;

        while (len > 1) {
            sum += *ptr++;
            len -= 2;
        }

        if (len == 1) {
            sum += *(uint8_t*)ptr;
        }

        // fold 32 -> 16
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        return static_cast<uint16_t>(~sum);
    }

    uint16_t ipv4_checksum(const uint32_t src_ip, const uint32_t dst_ip, const uint16_t protocol, const void *data, const int len) {
        const uint16_t checksum_size = sizeof(ipv4_PseudoHeader) + len;
        auto* checksum_buffer = static_cast<uint8_t *>(heap::malloc(checksum_size));
        auto* pseudo = reinterpret_cast<ipv4_PseudoHeader *>(checksum_buffer);

        pseudo->src_ip = Bswap_32(src_ip);
        pseudo->dst_ip = Bswap_32(dst_ip);
        pseudo->zero = 0;
        pseudo->protocol = protocol;
        pseudo->len = Bswap_16(len);

        memcpy(checksum_buffer + sizeof(ipv4_PseudoHeader), data, len);
        uint16_t ret = checksum(checksum_buffer, checksum_size);
        heap::free(checksum_buffer);

        if (ret == 0)
            ret = 0xFFFF;

        return ret;
    }
}
