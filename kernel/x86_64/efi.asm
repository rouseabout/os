bits 64
section .boot.header
align 4

extern bss
extern end

extern start2_64

global start
start:
    cli

    mov esp, boot_stack.bottom
    xor ebp, ebp

    jmp start2_64

section .boot.bss
align 4
boot_stack:
    resb 0x1000
.bottom:
