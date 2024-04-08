%ifdef ARCH_i686
bits 32
%elifdef ARCH_x86_64
bits 64
%endif

section .text

extern _libc_main
extern main

global _start
_start:

    mov ebp, 0
    call _libc_main

    mov ebx, eax
    mov eax, 4  ; OS_EXIT
    int 0x80
.loop:
    jmp .loop
