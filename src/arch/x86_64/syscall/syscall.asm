bits 64
extern dispatch_syscall
extern stack_top
extern user_rsp

global handle_syscall
section .text
handle_syscall:
    mov [user_rsp], rsp
    lea rsp, [rel stack_top]

    push rcx        ; user RIP
    push r11        ; user RFLAGS

    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi
    push rax

    sub rsp, 8      ; <-- padding to restore 16-byte alignment

    mov rdi, rsp
    add rdi, 8      ; struct pointer must skip the padding
    call dispatch_syscall

    add rsp, 8+7*8  ; undo padding + 7 regs
    pop r11
    pop rcx

    mov rsp, [user_rsp]
    o64 sysret


section .note.GNU-stack noalloc noexec nowrite progbits