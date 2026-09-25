#include "Process_manager.hpp"

#include "arch/x86_64/gdt/gdt.h"
#include "kernel/log.h"
#include "kernel/Memory/heap.hpp"

namespace proc {
    std::vector<Thread> Threads;
    int current_thread = -1;

    void create_thread(ThreadEntry func) {
        Thread thread = {};

        constexpr uint64_t STACK_SIZE = 1024 * 96; // 96KB

        thread.kernel_stack = reinterpret_cast<uint64_t>(heap::malloc_align(STACK_SIZE, 16)) + STACK_SIZE;
        thread.user_stack = reinterpret_cast<uint64_t>(heap::malloc_align(STACK_SIZE, 16)) + STACK_SIZE;

        auto* regs = reinterpret_cast<IDT::ISR_Registers*>(thread.kernel_stack - sizeof(IDT::ISR_Registers));

        *regs = {};

        regs->rip = reinterpret_cast<uint64_t>(func);
        regs->rsp = thread.user_stack;
        regs->cs = 0x30 | 3;
        regs->ss = 0x28 | 3;
        regs->rflags = 0x202;

        thread.regs = regs;

        Threads.push_back(std::move(thread));
    }

    void enter_ring3(Thread &thread) {
        tss.rsp0 = thread.kernel_stack;

        current_thread = 0;
        asm_ring3(thread.regs);
    }

    uint64_t schedule(IDT::ISR_Registers *regs) {
        if (current_thread < 0) return (uint64_t)regs;
        Threads[current_thread].regs = regs;

        current_thread++;

        if (current_thread >= Threads.size())
            current_thread = 0;

        Thread& next = Threads[current_thread];

        tss.rsp0 = next.kernel_stack;

        return (uint64_t)next.regs;
    }
}
