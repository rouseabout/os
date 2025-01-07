bits 32
section .boot.header
align 4

KERNEL_START equ 0xC0000000

extern bss
extern end

extern start2

global start
start:
    push dword 0x0
    popf

%ifdef ARCH_i686
    mov edi, bss - KERNEL_START
    mov ecx, end - KERNEL_START
    sub ecx, edi
    xor eax, eax
    rep stosb
%endif

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
