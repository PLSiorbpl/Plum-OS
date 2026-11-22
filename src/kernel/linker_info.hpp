#pragma once

namespace heap {
    struct Block;
}

namespace Linker {
    extern "C" char __kernel_start;

    extern "C" void (*__init_array_start[])();
    extern "C" void (*__init_array_end[])();

    // .text
    extern "C" char __kernel_text_start;
    extern "C" char __kernel_text_end;
    // .rodata
    extern "C" char __kernel_rodata_start;
    extern "C" char __kernel_rodata_end;

    // .data
    extern "C" char __kernel_data_start;
    extern "C" char __kernel_data_end;
    // .bss
    extern "C" char __kernel_bss_start;
    extern "C" char __kernel_bss_end;

    // .stack
    extern "C" char stack_bottom;
    extern "C" char stack_guard;
    extern "C" char stack_top;
    // .user_stack
    extern "C" char user_stack_bottom;
    extern "C" char user_stack_guard;
    extern "C" char user_stack_top;

    extern "C" char __kernel_end;
    extern "C" heap::Block __heap_start;
}
