%ifdef ARCH_i686
bits 32
%define OS_EXIT 1
%elifdef ARCH_x86_64
bits 64
%define OS_EXIT 60
%endif

section .text

extern _libc_main

global _start
_start:
%ifdef ARCH_i686
    mov ebp, 0
    pop esi   ; argc
    mov ecx, esp ; argv
    and esp, ~0xf
    push ecx ; argv
    push esi ; argc
%elifdef ARCH_x86_64
    mov rbp, 0
    pop rdi  ; argc
    lea rsi, [rsp] ; argv
    and rsp, ~0xf
%endif
    call _libc_main

    mov ebx, eax
    mov eax, OS_EXIT
    int 0x80
.loop:
    jmp .loop
