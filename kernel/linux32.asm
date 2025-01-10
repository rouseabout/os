bits 32
section .boot.header
align 4

extern start2

global start
start:
    push dword 0x0
    popf

    mov esp, boot_stack.bottom
    xor ebp, ebp

    push dword esi  ; linux_params
    push dword 0x1337  ; magic number
    call start2
    hlt


section .boot.bss
align 4
boot_stack:
    resb 0x1000
.bottom:
