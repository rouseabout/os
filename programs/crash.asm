global    _start

section   .text
_start:
    mov eax, 0xdead
    mov [eax], eax
