bits 32
section .text
%include "kernel/x86inc.asm"

global start2
extern start3
start2:
    ; identity map first 8MiB
    mov eax, pml3
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [pml4], eax

    mov eax, pml2
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [pml3], eax

    mov eax, pml1_0
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [pml2], eax

    mov eax, pml1_1
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [pml2 + 8], eax

    mov eax, pml1_2
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [pml2 + 16], eax

    mov eax, pml1_3
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [pml2 + 24], eax

    mov edi, pml1_0
    mov eax, PAGE_PRESENT | PAGE_WRITE
.loop:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    cmp eax, 0x800000
    jb .loop

    mov eax, 10100000b
    mov cr4, eax

    mov eax, pml4
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100 ; LME
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    add esp, 4
    pop dword edi ; magic
    pop dword esi ; info
    pop dword edx ; initial stack

    mov eax, tss
    mov [gdt.tss_entry + 2], ax
    shr eax, 16
    mov [gdt.tss_entry + 4], al
    shr eax, 8
    mov [gdt.tss_entry + 7], al

    lgdt [gdt_ptr]

    mov ax, 0x2b ; tss segment in gdt + rpl 3
    ltr ax

    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    jmp 0x8:start3

section .data

gdt:
    GDT_ENTRY32 0, 0, 0, 0
    GDT_ENTRY32 0, 0, 0x9A, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0x92, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0xFA, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0xF2, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
.tss_entry:
    GDT_ENTRY32 0, tss.end - tss - 1, 0xE9, 0
    dd 0, 0
.end:

align 4
    dw 0
gdt_ptr:
    dw gdt.end - gdt - 1
    dd gdt

align 16
tss:
    dd 0
    dq kernel_stack.bottom ; rsp
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0
    dw 0
    dw 0
.end

section .bss

pml4: dq 512 dup (?)
pml3: dq 512 dup (?)
pml2: dq 512 dup (?)
pml1_0: dq 512 dup (?)
pml1_1: dq 512 dup (?)
pml1_2: dq 512 dup (?)
pml1_3: dq 512 dup (?)

kernel_stack:
    db 0x1000 dup (?)
.bottom:
