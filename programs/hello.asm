%ifdef ARCH_i686
bits 32
%elifdef ARCH_x86_64
bits 64
%endif

global    _start

section   .text
_start:
    mov eax, 1 ;OS_WRITE
    mov ebx, 1 ;STDOUT_FILENO
    mov ecx, message
    mov edx, message.end - message
    int 0x80

    mov eax, 4 ;OS_EXIT
    xor ebx, ebx
    int 0x80

section .data
message:
    db "Hello World", 10
.end:
