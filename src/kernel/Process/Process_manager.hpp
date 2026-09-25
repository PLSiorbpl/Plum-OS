#pragma once
#include "std/types.hpp"
#include "std/vector.hpp"
#include "arch/x86_64/IDT/IDT.hpp"

namespace proc {
    struct Thread {
        IDT::ISR_Registers *regs;
        uint64_t kernel_stack;
        uint64_t user_stack;
    };

    extern int current_thread;
    using ThreadEntry = void(*)();

    extern std::vector<Thread> Threads;

    uint64_t schedule(IDT::ISR_Registers *regs);

    inline Thread *get_thread() {
        return &Threads[current_thread];
    }

    void create_thread(ThreadEntry func);
    void enter_ring3(Thread &thread);
    extern "C" void asm_ring3(IDT::ISR_Registers *regs);
}
