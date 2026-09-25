global asm_ring3

asm_ring3:
    mov rbx, rdi
    mov r15, [rbx + 0x00]
    mov r14, [rbx + 0x08]
    mov r13, [rbx + 0x10]
    mov r12, [rbx + 0x18]
    mov r11, [rbx + 0x20]
    mov r10, [rbx + 0x28]
    mov r9, [rbx + 0x30]
    mov r8, [rbx + 0x38]
    mov rsi, [rbx + 0x48]
    mov rbp, [rbx + 0x50]
    mov rdx, [rbx + 0x60]
    mov rcx, [rbx + 0x68]
    mov rax, [rbx + 0x70]

    mov rcx, [rbx + 136]
    mov r11, [rbx + 152]

    ; User stack
    mov rsp, [rbx + 160]
    and rsp, ~0xF
    sub rsp, 8

    ; Segments
    mov ax, 0x2B
    mov ds, ax
    mov es, ax

    o64 sysret ; kernel -> user