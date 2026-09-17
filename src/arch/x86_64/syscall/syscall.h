#pragma once
#include "Drivers/Keyboard.hpp"
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "Drivers/Network/IPv4/TCP.hpp"
#include "Drivers/Network/Sockets/tcp_socket.hpp"

enum class syscall_id : u64 {
    write = 0,
    put_char = 1,
    serial_write = 2,
    serial_put_char = 3,
    get_key = 4,
    exit = 5,
    sleep = 6,
    pci = 7,
    heap = 8,
    swap_framebuffer = 9,
    list_partitions = 10,
    USB = 20,
    OpenPL = 21,
    socket = 22,
    tcp_socket = 23,
};

struct syscall_regs {
    u64 id; // rax
    u64 arg1; // rdi
    u64 arg2; // rsi
    u64 arg3; // rdx
    u64 arg4; // r10
    u64 arg5; // r8
    u64 arg6; // r9
};

inline u64 syscall(u64 id, u64 a1 = 0, u64 a2 = 0, u64 a3 = 0, u64 a4 = 0, u64 a5 = 0, u64 a6 = 0) {
    register u64 r10 asm("r10") = a4;
    register u64 r8  asm("r8")  = a5;
    register u64 r9  asm("r9")  = a6;

    u64 ret;
    asm volatile("syscall"
        : "=a"(ret)
        : "a"(id),
          "D"(a1),
          "S"(a2),
          "d"(a3),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "rcx", "r11", "memory"
    );

    return ret;
}

inline u64 sys_write(const char* str, const u64 color) {
    return syscall(0, reinterpret_cast<u64>(str), color);
}

inline u64 sys_put_char(const char c, const u64 color) {
    return syscall(1, (u64)static_cast<u8>(c), color);
}

inline u64 sys_serial_write(const char* c) {
    return syscall(2, reinterpret_cast<u64>(c));
}

inline u64 sys_serial_put_char(const char c) {
    return syscall(3, (u64)static_cast<u8>(c));
}

inline kb::key_code sys_get_key(const bool wait = true) {
    return static_cast<kb::key_code>(syscall(4, (u64)wait));
}

inline void sys_exit() {
    syscall(5);
}

inline void sys_sleep(const u64 milliseconds) {
    syscall(6, milliseconds);
}

inline void sys_pci_test() {
    syscall(7);
}

inline void sys_heap_dump(const bool show_all) {
    syscall(8, show_all);
}

inline void sys_swap_framebuffer() {
    syscall(9);
}

inline void sys_list_parts() {
    syscall(10);
}

inline void sys_usb() {
    syscall(static_cast<u64>(syscall_id::USB));
}

inline void sys_openPL(OpenPL::Context *ctx, const uint32_t Operation) {
    syscall(21, reinterpret_cast<u64>(ctx), Operation);
}

// 0 - receive   1 - send
inline bool sys_socket(int socket, void *recv, bool dir, int timeout = 0) {
    return syscall(22, static_cast<u64>(socket), reinterpret_cast<u64>(recv), (u64)dir, static_cast<u64>(timeout));
}

// 0 - receive   1 - send
inline bool sys_tcp_socket(tsock::tcp_syscall *buf, bool dir, int timeout = 0) {
    return syscall(23, reinterpret_cast<u64>(buf), dir, static_cast<u64>(timeout));
}