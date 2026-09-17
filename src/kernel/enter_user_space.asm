bits 64

global enter_user_space

extern user_space_main
extern user_stack_top
extern handle_syscall

section .text
enter_user_space:
    ; Set syscall handler
    mov ecx, 0xC0000082
    mov rax, handle_syscall
    mov rdx, rax
    shr rdx, 32
    wrmsr

    ; enable syscall
    mov ecx, 0xC0000080 ; EFER
    rdmsr
    or eax, 1
    wrmsr

    ; FMASK
    mov ecx, 0xC0000084
    xor edx, edx
    mov eax, 0x200 ; Interrupt flag
    wrmsr

    ; STAR
    mov ecx, 0xC0000081
    xor eax, eax
    ; 0x20 Kernel Data  0x18 Kernel Code       64bit
    mov edx, (0x20 << 16) | 0x18
    wrmsr

    ; Preparing user sapace ig
    mov rcx, user_space_main
    mov r11, 0x202

    ; User stack
    mov rax, user_stack_top
    mov rsp, rax
    and rsp, ~0xF
    sub rsp, 8

    ; Segments
    mov ax, 0x2B
    mov ds, ax
    mov es, ax

    o64 sysret ; kernel -> user

section .note.GNU-stack noalloc noexec nowrite progbits