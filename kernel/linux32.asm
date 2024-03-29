bits 32
section .text
align 4

extern start2

global multiboot
multiboot:

global start
start:
    mov esp, sys_stack_top
    xor ebp, ebp

    push esp  ; initial stack
    push dword esi  ; linux_params
    push dword 0x1337  ; magic number
    call start2
    hlt


section .bss
align 4
    resb 0x1000
sys_stack_top:
