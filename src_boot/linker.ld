OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kernel_main)

PHDRS
{
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS
{
    . = 0xffffffff80000000;
    __kernel_start = .;
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

	. = ALIGN(4K);
	.text : {
	    __kernel_text_start = .;
	    *(.text*)
	    __kernel_text_end = .;
	} :text

    . = ALIGN(4K);
	.rodata : {
	    __kernel_rodata_start = .;
	    *(.rodata*)
	    __kernel_rodata_end = .;
	} :rodata

    . = ALIGN(4K);
	.data : {
	    __kernel_data_start = .;
	    *(.data*)
	    __kernel_data_end = .;
	} :data

    . = ALIGN(4K);
	.bss  : {
	__kernel_bss_start = .;
	    *(.bss*)
	    __kernel_bss_end = .;
	} :bss

    . = ALIGN(4K);
    .stack (NOLOAD) : {
        . = ALIGN(16);
        stack_bottom = .;
        . += 64K; /* so stack overflows dont delete stuf */
        stack_guard = .;
        . += 16K;
        stack_top = .;
    } :bss

    . = ALIGN(4K);
    .user_stack (NOLOAD) : {
        . = ALIGN(16);
        user_stack_bottom = .;
        . += 64K; /* so stack overflows dont delete stuf */
        user_stack_guard = .;
        . += 16K;
        user_stack_top = .;
    } :bss

	__kernel_end = .;
	. = ALIGN(16);
	__heap_start = .;
}