#include "syscall.h"

#include <kernel/Sleep.hpp>

#include "arch/x86_64/Common/Common.hpp"
#include "Drivers/Keyboard.hpp"
#include "Drivers/PCI.hpp"
#include "Drivers/GPU/framebuffer.hpp"
#include "Drivers/USB/xHCI/xHCI.hpp"
#include "kernel/system.hpp"
#include "kernel/Memory/heap.hpp"
#include "../../../Drivers/Network/Sockets/socket.hpp"
#include "Drivers/Network/Drivers/RTL8139.hpp"
#include "Drivers/Network/IPv4/TCP.hpp"
#include "kernel/log.h"

extern "C" u64 user_rsp = 0;

extern "C" u64 user_rcx = 0;
extern "C" u64 user_r11 = 0;

auto validate_user_ptr = [](const u64 ptr) -> bool {
    return ptr != 0;
};

extern "C" u64 dispatch_syscall(syscall_regs *regs) {
    switch (static_cast<syscall_id>(regs->id)) {
        case syscall_id::write:
            if (!validate_user_ptr(regs->arg1))
                return static_cast<u64>(-1);
            for (int i = 0; reinterpret_cast<const char*>(regs->arg1)[i] != '\0'; i++) {
                systemPL::fb.put_char(reinterpret_cast<const char*>(regs->arg1)[i], color_to_rgb(static_cast<Color>(regs->arg2)));
            }
            return 0;

        case syscall_id::put_char:
            systemPL::fb.put_char(static_cast<char>(regs->arg1), color_to_rgb(static_cast<Color>(regs->arg2)));
            return 0;

        case syscall_id::serial_put_char:
            while (!(x64::inb(0x3F8 + 5) & 0x20)) { }
            x64::outb(0x3F8, static_cast<char>(regs->arg1));
            return 0;

        case syscall_id::serial_write: {
            if (!validate_user_ptr(regs->arg1))
                return static_cast<u64>(-1);
            const auto text = reinterpret_cast<const char *>(regs->arg1);
            for (int i = 0; text[i] != '\0'; i++) {
                while (!(x64::inb(0x3F8 + 5) & 0x20)) { }
                x64::outb(0x3F8, text[i]);
            }
            return 0;
        }

	    case syscall_id::get_key:
            if (regs->arg1 == true) {
                return static_cast<u64>(kb::get_char());
            }
            return static_cast<u64>(kb::read_char());

        case syscall_id::exit:
            return 0;

        case syscall_id::sleep:
            systemPL::fb.swap(); // TODO remove all the swaps everywhere and just keep swapping at a fixed itnerval on a separate thread when we have threads
            Time::Sleep(regs->arg1);
            return 0;

        case syscall_id::pci:
            PCI::Test();
            return 0;

        case syscall_id::heap:
            heap::dump_heap(regs->arg1);
            return 0;

        case syscall_id::swap_framebuffer:
            systemPL::fb.swap();
            return 0;

        case syscall_id::list_partitions:
            systemPL::partition_manager.list_partitions();
            return 0;

        case syscall_id::USB:
            USB::m_xhci_driver.process_pending_port_changes();
            return 0;

        case syscall_id::OpenPL: {
            const auto ctx = reinterpret_cast<OpenPL::Context *>(regs->arg1);
            ctx->Swap();
            return 0;
        }
        case syscall_id::socket: {
            if (static_cast<bool>(regs->arg3) == false)
                return soc::recv(static_cast<int>(regs->arg1), *reinterpret_cast<soc::udp_recv_packet *>(regs->arg2), static_cast<int>(regs->arg4));
            return soc::sendto(static_cast<int>(regs->arg1), *reinterpret_cast<soc::udp_send_packet *>(regs->arg2));
        }
        case syscall_id::tcp_socket: {
            const auto tcp = reinterpret_cast<tsock::tcp_syscall *>(regs->arg1);
            if (static_cast<bool>(regs->arg2) == false)
                return 0;
            NET::send_tcp(&RTL8139::driver, tcp->sock, tcp->flags, tcp->data, tcp->size);
            return 0;
        }
        default:
            return static_cast<u64>(-1); // ENOSYS
    }
}
