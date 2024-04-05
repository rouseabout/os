bits 32
section .text
%include "kernel/x86inc.asm"

extern start3
global start2
start2:
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
    dw 0
gdt_ptr:
    dw gdt.end - gdt - 1
    dd gdt

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
    db 0x1000 dup (?)
.bottom:
