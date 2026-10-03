bits 64
global asm_ring3
global asm_restore_frame

%define FX 512

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

asm_ring3:
    mov rbx, rdi

    fxrstor [rbx]

    mov r15, [rbx + FX + 0x00]
    mov r14, [rbx + FX + 0x08]
    mov r13, [rbx + FX + 0x10]
    mov r12, [rbx + FX + 0x18]
    mov r11, [rbx + FX + 0x20]
    mov r10, [rbx + FX + 0x28]
    mov r9, [rbx + FX + 0x30]
    mov r8, [rbx + FX + 0x38]
    mov rdi, [rbx + FX + 0x40]
    mov rsi, [rbx + FX + 0x48]
    mov rbp, [rbx + FX + 0x50]
    mov rdx, [rbx + FX + 0x60]
    mov rcx, [rbx + FX + 0x68]
    mov rax, [rbx + FX + 0x70]

    mov rcx, [rbx + FX + 136]   ; rip
    mov r11, [rbx + FX + 152]   ; rflags

    ; User stack
    mov rsp, [rbx + FX + 160]   ; rsp
    and rsp, ~0xF
    sub rsp, 8

    mov rbx, [rbx + FX + 88]

    ; Segments
    mov ax, 0x2B
    mov ds, ax
    mov es, ax

    o64 sysret ; kernel -> user

asm_restore_frame:
    mov rsp, rdi
    fxrstor [rsp]
    add rsp, FX
    popall
    add rsp, 16
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits