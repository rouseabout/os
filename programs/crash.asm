%ifdef ARCH_i686
bits 32
%elifdef ARCH_x86_64
bits 64
%endif

global    _start

section   .text
_start:
    mov eax, 0xdead
    mov [eax], eax
