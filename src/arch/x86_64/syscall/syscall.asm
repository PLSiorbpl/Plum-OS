bits 64
extern dispatch_syscall
extern user_rsp
extern current_kernel_stack

global handle_syscall
section .text
handle_syscall:
    push r12
    mov r12, rsp ; save User RSP
    mov rsp, [current_kernel_stack] ; load current thread's kernel RSP

    push r12        ; user RSP
    push rcx        ; user RIP
    push r11        ; user RFLAGS

    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi
    push rax

    push rbx        ; callee-saved regs — dispatch_syscall is C++ and may clobber them,
    push rbp        ; the user thread must get them back after sysret
    push r13
    push r14
    push r15

    sub rsp, 8      ; padding; rsp now 16-byte aligned for the call

    mov rdi, rsp
    add rdi, 8+5*8  ; struct pointer must skip the padding + callee-saved regs
    call dispatch_syscall

    add rsp, 8      ; undo padding
    pop r15
    pop r14
    pop r13
    pop rbp
    pop rbx

    add rsp, 7*8    ; undo 7 syscall-arg regs
    pop r11
    pop rcx
    pop rsp

    pop r12

    o64 sysret


section .note.GNU-stack noalloc noexec nowrite progbits