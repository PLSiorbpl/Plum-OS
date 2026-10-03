#pragma once
#include "std/types.hpp"
#include "std/vector.hpp"
#include "arch/x86_64/IDT/IDT.hpp"

namespace proc {
    enum class ThreadState : uint8_t {
        Ready,
        Running,
        Blocked,
        Terminated
    };

    struct Thread {
        IDT::ISR_Registers *regs;
        uint64_t kernel_stack;
        uint64_t user_stack;

        ThreadState state = ThreadState::Ready;

        uint8_t priority = 3;
        uint8_t work = 0;
    };
    extern std::vector<Thread> Threads;

    extern int current_thread;
    extern "C" uint64_t current_kernel_stack;
    using ThreadEntry = void(*)();

    uint64_t schedule(IDT::ISR_Registers *regs);

    inline Thread *get_thread() {
        if (current_thread < 0) return nullptr;
        return &Threads[current_thread];
    }

    void create_thread(ThreadEntry func, uint8_t priority = 3);
    void enter_ring3(const Thread &thread);
    extern "C" void asm_ring3(IDT::ISR_Registers *regs);

    [[noreturn]] void thread_wrapper(ThreadEntry entry);
    void stub_thread();
}
