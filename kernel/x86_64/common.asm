bits 64
section .text

%macro DEFISR 1
global isr%1
isr%1:
    cli
    push byte 0
    push %1
    jmp isr_common
%endmacro

%macro DEFISR_ERRORCODE 1
global isr%1
isr%1:
    cli
    push %1
    jmp isr_common
%endmacro

%macro DEFIRQ 1
global irq%1
irq%1:
    cli
    push byte 0
    push 32 + %1
    jmp isr_common
%endmacro

DEFISR 0
DEFISR 1
DEFISR 2
DEFISR 3
DEFISR 4
DEFISR 5
DEFISR 6
DEFISR 7
DEFISR_ERRORCODE 8
DEFISR 9
DEFISR_ERRORCODE 10
DEFISR_ERRORCODE 11
DEFISR_ERRORCODE 12
DEFISR_ERRORCODE 13
DEFISR_ERRORCODE 14
DEFISR 15
DEFISR 16
DEFISR_ERRORCODE 17
DEFISR 18
DEFISR 19
DEFISR 20
DEFISR_ERRORCODE 21 ;;??errorcode
DEFISR 22
DEFISR 23
DEFISR 24
DEFISR 25
DEFISR 26
DEFISR 27
DEFISR 28
DEFISR 29
DEFISR 30  ;; ??errorcode
DEFISR 31

DEFIRQ 0
DEFIRQ 1
DEFIRQ 2
DEFIRQ 3
DEFIRQ 4
DEFIRQ 5
DEFIRQ 6
DEFIRQ 7
DEFIRQ 8
DEFIRQ 9
DEFIRQ 10
DEFIRQ 11
DEFIRQ 12
DEFIRQ 13
DEFIRQ 14
DEFIRQ 15

DEFISR 128

extern interrupt_handler

isr_common:
    cld

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi

    mov rbp, ds ; save user-space data segment
    push rbp

    mov rbp, 0x10 ; set kernel-space data segment
    mov ds, rbp
    mov es, rbp

    mov rdi, rsp
    call interrupt_handler

    pop rbp ; restore user-space data segment
    mov ds, rbp
    mov es, rbp

    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16
    iretq


global jmp_to_userspace
jmp_to_userspace:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23
    push rsi
    pushf
    push 0x1b
    push rdi
    iretq
