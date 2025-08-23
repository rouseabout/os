%ifdef ARCH_i686
bits 32
%define OS_EXIT 1
%elifdef ARCH_x86_64
bits 64
%define OS_EXIT 60
%endif

section .text

extern _libc_main
extern main

global _start
_start:

    mov ebp, 0
    call _libc_main

    mov ebx, eax
    mov eax, OS_EXIT
    int 0x80
.loop:
    jmp .loop
