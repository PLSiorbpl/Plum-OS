#pragma once
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "Drivers/Network/Sockets/tcp_socket.hpp"
#include "kernel/Process/FileDescriptor.hpp"

enum class syscall_id : u64 {
    read = 0,
    write = 1,
    open = 2,
    stat = 4,
    fstat = 5,
    lstat = 6,
    lseek = 8,
    nanosleep = 35,
    exit = 60,
    truncate = 76,
    ftruncate = 77,

    pci = 100,
    heap = 101,
    swap_framebuffer = 102,
    list_partitions = 103,
    USB = 104,
    OpenPL = 105,
    socket = 106,
    tcp_socket = 107,
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

inline u64 sys_write(const unsigned int fd, const char* buf, const size_t count) {
    return syscall((u64)syscall_id::write, fd, reinterpret_cast<u64>(buf), count);
}

inline u64 sys_read(const unsigned int fd, const char *buf, const size_t count) {
    return syscall((u64)syscall_id::read, fd, reinterpret_cast<u64>(buf), count);
}

inline u64 sys_open(const char *filename, const int flags, const int mode) {
    return syscall((u64)syscall_id::open, reinterpret_cast<u64>(filename), flags, mode);
}

inline u64 sys_stat(const char *filename, vfs::FileStat *statbuf) {
    return syscall((u64)syscall_id::stat, reinterpret_cast<u64>(filename), reinterpret_cast<u64>(statbuf));
}

inline u64 sys_fstat(const unsigned int fd, vfs::FileStat *statbuf) {
    return syscall((u64)syscall_id::fstat, fd, reinterpret_cast<u64>(statbuf));
}

inline u64 sys_lstat(const char *filename, vfs::FileStat *statbuf) {
    return syscall((u64)syscall_id::lstat, reinterpret_cast<u64>(filename), reinterpret_cast<u64>(statbuf));
}

inline u64 sys_lseek(const unsigned int fd, const int64_t offset, const unsigned int whence) {
    return syscall((u64)syscall_id::lseek, fd, offset, whence);
}

inline void sys_exit(const int status) {
    syscall(static_cast<u64>(syscall_id::exit), status);
}

inline void sys_sleep(const u64 milliseconds) {
    syscall(static_cast<u64>(syscall_id::nanosleep), milliseconds);
}

inline u64 sys_truncate(const char *filename, uint64_t lenght) {
    return syscall((u64)syscall_id::ftruncate, reinterpret_cast<u64>(filename), lenght);
}

inline u64 sys_ftruncate(const unsigned int fd, uint64_t lenght) {
    return syscall((u64)syscall_id::ftruncate, fd, lenght);
}

inline void sys_pci_test() {
    syscall(static_cast<u64>(syscall_id::pci));
}

inline void sys_heap_dump(const bool show_all) {
    syscall(static_cast<u64>(syscall_id::heap), show_all);
}

inline void sys_swap_framebuffer() {
    syscall(static_cast<u64>(syscall_id::swap_framebuffer));
}

inline void sys_list_parts() {
    syscall(static_cast<u64>(syscall_id::list_partitions));
}

inline void sys_usb() {
    syscall(static_cast<u64>(syscall_id::USB));
}

inline void sys_openPL(OpenPL::Context *ctx, const uint32_t Operation) {
    syscall(static_cast<u64>(syscall_id::OpenPL), reinterpret_cast<u64>(ctx), Operation);
}

// 0 - receive   1 - send
inline bool sys_socket(int socket, void *recv, bool dir, int timeout = 0) {
    return syscall(static_cast<u64>(syscall_id::socket), static_cast<u64>(socket), reinterpret_cast<u64>(recv), (u64)dir, static_cast<u64>(timeout));
}

// 0 - receive   1 - send
inline bool sys_tcp_socket(tsock::tcp_syscall *buf, bool dir, int timeout = 0) {
    return syscall(static_cast<u64>(syscall_id::tcp_socket), reinterpret_cast<u64>(buf), dir, static_cast<u64>(timeout));
}