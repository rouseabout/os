%ifdef ARCH_i686
bits 32
%define OS_WRITE 4
%define OS_EXIT 1
%elifdef ARCH_x86_64
bits 64
%define OS_WRITE 1
%define OS_EXIT 60
%endif

global    _start

section   .text
_start:
%ifdef ARCH_i686
    mov eax, OS_WRITE
    mov ebx, 1 ;STDOUT_FILENO
    mov ecx, message
    mov edx, message.end - message
    int 0x80

    mov eax, OS_EXIT
    xor ebx, ebx
    int 0x80
%elifdef ARCH_x86_64
    mov rax , OS_WRITE
    mov rdi, 1 ;STDOUT_FILENO
    mov rsi, message
    mov rdx, message.end - message
    syscall

    mov rax, OS_EXIT
    xor rdi, rdi
    syscall
%endif

section .data
message:
    db "Hello World", 10
.end:
