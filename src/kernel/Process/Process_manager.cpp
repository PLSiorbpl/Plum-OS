#include "Process_manager.hpp"

#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/syscall/syscall.h"
#include "kernel/log.h"
#include "kernel/Memory/heap.hpp"

namespace proc {
    uint64_t current_kernel_stack;
    std::vector<Thread> Threads;
    int current_thread = -1;

    void create_thread(ThreadEntry func, uint8_t priority) {
        if (!func)
            func = stub_thread;
        Thread thread = {};

        constexpr uint64_t STACK_SIZE = 1024 * 96; // 96KB

        thread.kernel_stack = reinterpret_cast<uint64_t>(heap::malloc_align(STACK_SIZE, 16)) + STACK_SIZE;
        thread.user_stack = reinterpret_cast<uint64_t>(heap::malloc_align(STACK_SIZE, 16)) + STACK_SIZE;

        auto* regs = reinterpret_cast<IDT::ISR_Registers*>(thread.kernel_stack - sizeof(IDT::ISR_Registers));

        *regs = {};

        regs->rip = reinterpret_cast<uint64_t>(thread_wrapper);
        regs->rdi = reinterpret_cast<uint64_t>(func);
        regs->rsp = thread.user_stack - 8;
        regs->cs = 0x30 | 3;
        regs->ss = 0x28 | 3;
        regs->rflags = 0x202;

        thread.regs = regs;
        thread.state = ThreadState::Running;
        thread.priority = priority;
        thread.work = 0;

        Threads.push_back(thread);
    }

    void enter_ring3(const Thread &thread) {
        tss.rsp0 = thread.kernel_stack;
        current_kernel_stack = thread.kernel_stack;

        current_thread = 0;
        asm_ring3(thread.regs);
    }

    uint64_t schedule(IDT::ISR_Registers *regs) {
        if (current_thread < 0) return reinterpret_cast<uint64_t>(regs);
        Threads[current_thread].regs = regs;

        current_thread++;

        if (current_thread >= Threads.size())
            current_thread = 0;

        Thread& next = Threads[current_thread];
        next.work = 0;

        current_kernel_stack = next.kernel_stack;
        tss.rsp0 = next.kernel_stack;

        log::info("%u", current_thread);
        return reinterpret_cast<uint64_t>(next.regs);
    }

    void thread_wrapper(ThreadEntry entry) {
        entry();

        sys_exit(0);
        while (true) { asm volatile("pause"); }

        __builtin_unreachable();
    }

    void stub_thread() {
        std::printf("yes");
        sys_exit(0);
        std::printf("yes");
        return;
    }
}
