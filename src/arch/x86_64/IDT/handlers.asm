bits 64

section .text

%macro pushall 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

%macro ISR_NOERR 1
[global isr%1]
isr%1:
    push qword 0 ; dummy error code
    push qword %1 ; interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
[global isr%1]
isr%1:
    ; error code is pushed by cpu
    push qword %1 ; interrupt number
    jmp isr_common_stub
%endmacro

%assign i 0
%rep 32
    ; exceptions
    %if i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
%assign i i+1
%endrep

%assign i 32
%rep 224
    ISR_NOERR i
%assign i i+1
%endrep

isr_common_stub:
    pushall

    mov rdi, rsp ; first argument
    mov rbp, rsp ; stack alligment
    and rsp, -16

    ; Save XMM registers (16 regs × 16 bytes = 256 bytes)
    sub rsp, 256
    movdqu [rsp + 0x00], xmm0
    movdqu [rsp + 0x10], xmm1
    movdqu [rsp + 0x20], xmm2
    movdqu [rsp + 0x30], xmm3
    movdqu [rsp + 0x40], xmm4
    movdqu [rsp + 0x50], xmm5
    movdqu [rsp + 0x60], xmm6
    movdqu [rsp + 0x70], xmm7
    movdqu [rsp + 0x80], xmm8
    movdqu [rsp + 0x90], xmm9
    movdqu [rsp + 0xA0], xmm10
    movdqu [rsp + 0xB0], xmm11
    movdqu [rsp + 0xC0], xmm12
    movdqu [rsp + 0xD0], xmm13
    movdqu [rsp + 0xE0], xmm14
    movdqu [rsp + 0xF0], xmm15

    ; Save MXCSR (SSE control/status register)
    sub rsp, 16        ; aligned space for stmxcsr
    stmxcsr [rsp]

    extern isr_common
    call isr_common

    ; Restore MXCSR
    ldmxcsr [rsp]
    add rsp, 16

    ; Restore XMM registers
    movdqu xmm0,  [rsp + 0x00]
    movdqu xmm1,  [rsp + 0x10]
    movdqu xmm2,  [rsp + 0x20]
    movdqu xmm3,  [rsp + 0x30]
    movdqu xmm4,  [rsp + 0x40]
    movdqu xmm5,  [rsp + 0x50]
    movdqu xmm6,  [rsp + 0x60]
    movdqu xmm7,  [rsp + 0x70]
    movdqu xmm8,  [rsp + 0x80]
    movdqu xmm9,  [rsp + 0x90]
    movdqu xmm10, [rsp + 0xA0]
    movdqu xmm11, [rsp + 0xB0]
    movdqu xmm12, [rsp + 0xC0]
    movdqu xmm13, [rsp + 0xD0]
    movdqu xmm14, [rsp + 0xE0]
    movdqu xmm15, [rsp + 0xF0]
    add rsp, 256

    mov rsp, rbp ; original stack
    popall
    add rsp, 16 ; remove vector and number
    iretq

section .data
    global isr_table

    isr_table:
    %assign i 0
    %rep 256
        dq isr %+ i
    %assign i i+1
    %endrep

section .note.GNU-stack noalloc noexec nowrite progbits