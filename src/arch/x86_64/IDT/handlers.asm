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

    mov rdi, rsp        ; frame pointer for isr_common — before any alignment changes
    mov rbp, rsp        ; save for restoration
    and rsp, -16        ; align

    ; FXSAVE needs 512 bytes, 16-byte aligned
    sub rsp, 512
    fxsave [rsp]

    extern isr_common
    call isr_common

    fxrstor [rsp]
    add rsp, 512

    mov rsp, rbp
    popall
    add rsp, 16
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