bits 32
section .boot.text
%include "kernel/x86inc.asm"

KERNEL_START equ 0xC000000
PML1_SIZE equ 0x40000 ; 4MiB

extern start3
global start2
start2:

%macro SET_PAGE 3 ; table index phy
    mov eax, %3
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [%1 + %2 * 4], eax
%endmacro

    ; identity map first 8MiB and mirror first 8MiB @ KERNEL_START

    SET_PAGE pml2, 0, pml1_0
    SET_PAGE pml2, 1, pml1_1
    SET_PAGE pml2, KERNEL_START / PML1_SIZE, pml1_0
    SET_PAGE pml2, (KERNEL_START / PML1_SIZE) + 1, pml1_1

    mov edi, pml1_0
    mov eax, PAGE_PRESENT | PAGE_WRITE
.loop:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    cmp eax, 0x800000
    jb .loop

    mov eax, pml2
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

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

    ; copy arguments from boot stack to kernel stack
    add esp, 4
    pop eax ; magic
    pop ebx ; info
    mov esp, kernel_stack.bottom
    xor ebp, ebp
    push dword ebx ; info
    push dword eax ; magic
    push dword $

    jmp 0x8:start3

section .boot.data

align 4
    dw 0
gdt_ptr:
    dw gdt.end - gdt - 1
    dd gdt

section .boot.bss

pml2: dd 1024 dup (0)
pml1_0: dd 1024 dup (0)
pml1_1: dd 1024 dup (0)

section .data

align 4
gdt:
    GDT_ENTRY32 0, 0, 0, 0
    GDT_ENTRY32 0, 0xFFFFFFFF, 0x9A, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0xFFFFFFFF, 0x92, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0xFFFFFFFF, 0xFA, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0xFFFFFFFF, 0xF2, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
.tss_entry:
    GDT_ENTRY32 0, tss.end - tss - 1, 0xE9, 0
.end:

align 4
tss:
    dd 0 ; prev_tss;
    dd kernel_stack.bottom ; esp0
    dd 0x10 ; ss0;
    dd 0 ; esp1;
    dd 0 ; ss1;
    dd 0 ; esp2;
    dd 0 ; ss2;
    dd 0 ; cr3;
    dd 0 ; eip;
    dd 0 ; eflags;
    dd 0 ; eax;
    dd 0 ; ecx;
    dd 0 ; edx;
    dd 0 ; ebx;
    dd 0 ; esp;
    dd 0 ; ebp;
    dd 0 ; esi;
    dd 0 ; edi;
    dd 0x13 ; es; kernel data segment (0x10) + requested privilege level (3)
    dd 0xb ; cs; kernel code segment (0x08) + 'requested privellege level' (3)
    dd 0x13 ; ss;
    dd 0x13 ; ds;
    dd 0x13 ; fs;
    dd 0x13 ; gs;
    dd 0 ; ldt;
    dw 0 ; trap;
    dw 0 ; iomap_base;
.end:

section .bss

kernel_stack:
    resb 0x1000
.bottom:
