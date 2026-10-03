#include "Process_manager.hpp"

#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/syscall/syscall.h"
#include "kernel/log.h"
#include "kernel/Memory/heap.hpp"

extern "C" void asm_restore_frame(uint64_t frame);

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

        *reinterpret_cast<uint16_t*>(&regs->fx[0]) = 0x037F; // x87 control word
        *reinterpret_cast<uint32_t*>(&regs->fx[24]) = 0x1F80; // MXCSR

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

    int pick_next_runnable() {
        if (Threads.empty()) return -1;

        const int start = current_thread < 0 ? 0 : current_thread;
        for (int i = 1; i <= static_cast<int>(Threads.size()); i++) {
            const int idx = (start + i) % static_cast<int>(Threads.size());
            const ThreadState s = Threads[idx].state;
            if (s != ThreadState::Terminated && s != ThreadState::Blocked)
                return idx;
        }
        return -1;
    }

    uint64_t schedule(IDT::ISR_Registers *regs) {
        if (current_thread < 0) return reinterpret_cast<uint64_t>(regs);
        Threads[current_thread].regs = regs;

        const int next = pick_next_runnable();
        if (next < 0) return reinterpret_cast<uint64_t>(regs);

        current_thread = next;
        Threads[next].work = 0;

        current_kernel_stack = Threads[next].kernel_stack;
        tss.rsp0 = Threads[next].kernel_stack;

        return reinterpret_cast<uint64_t>(Threads[next].regs);
    }

    [[noreturn]] void exit_current() {
        if (current_thread >= 0 && current_thread < static_cast<int>(Threads.size()))
            Threads[current_thread].state = ThreadState::Terminated;

        const int next = pick_next_runnable();
        if (next < 0) { // nothing left to run
            current_thread = -1;
            for (;;) asm volatile("sti; hlt");
        }

        current_thread = next;
        current_kernel_stack = Threads[next].kernel_stack;
        tss.rsp0 = Threads[next].kernel_stack;

        asm_restore_frame(reinterpret_cast<uint64_t>(Threads[next].regs));
        __builtin_unreachable();
    }

    [[noreturn]] void thread_wrapper(ThreadEntry entry) {
        entry();

        exit_current();

        __builtin_unreachable();
    }

    void stub_thread() {

    }
}
