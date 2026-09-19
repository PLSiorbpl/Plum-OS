#pragma once
#include "std/Ring_Buffer.hpp"
#include "std/types.hpp"
#include "std/vector.hpp"
#include "std/unique.hpp"

namespace tsock {
    enum class tcp_state {
        CLOSED,
        LISTEN,
        SYN_SENT,
        SYN_RECEIVED,
        ESTABLISHED,
        FIN_WAIT_1,
        FIN_WAIT_2,
        CLOSE_WAIT,
        CLOSING,
        LAST_ACK,
        TIME_WAIT
    };

    struct cdata {
        void *data = nullptr;
        size_t size = 0;
    };

    class tcp_socket {
    public:
        bool bind(uint16_t port);
        bool listen();
        tcp_socket* accept();

        //bool connect(uint32_t ip, uint16_t port);

        size_t send(const void* data, size_t len);
        size_t recv(void* data, size_t len);

        void close();

        std::Ring_Buffer<cdata, 128> rx_queue;

        int num = 0;

        uint16_t local_port = 0;
        uint32_t remote_ip = 0;
        uint16_t remote_port = 0;

        tcp_state state = {};

        uint32_t send_seq = 0;
        uint32_t recv_seq = 0;

        std::vector<tcp_socket *> pending;
    };

    struct tcp_syscall {
        tcp_socket *sock;
        uint8_t flags;
        uint8_t *data;
        size_t size;
    };

    bool remove_socket(int sock);
    tcp_socket *find_socket(int sock);
    tcp_socket *find_socket_listner(uint16_t local_port);
    tcp_socket *find_socket_connection(uint16_t local_port, uint32_t remote_ip, uint16_t remote_port);

    int socket();

    extern std::vector<std::unique_ptr<tcp_socket>> sockets;
}