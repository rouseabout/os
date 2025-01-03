bits 32
section .boot.text
%include "kernel/x86inc.asm"

KERNEL_START equ 0x8000000000
PML123_SIZE equ 0x8000000000 ; 512 GiB

extern bss
extern end

global start2
global start2_64
extern start3
start2:

%macro SET_PAGE 3 ; table index phy
    mov eax, %3
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [%1 + (%2) * 8], eax
%endmacro

    ; identity map first 8MiB and mirror first 8MiB @ KERNEL_START

    SET_PAGE pml4, 0, pml3
    SET_PAGE pml4, KERNEL_START / PML123_SIZE, pml3

    SET_PAGE pml3, 0, pml2

    SET_PAGE pml2, 0, pml1_0
    SET_PAGE pml2, 1, pml1_1
    SET_PAGE pml2, 2, pml1_2
    SET_PAGE pml2, 3, pml1_3

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
    or eax, 0x80000001 ; pg,pe
    and eax, ~0x10000 ; wp
    mov cr0, eax

    add esp, 4
    pop dword edi ; magic
    pop dword esi ; info

    lgdt [gdt_ptr]

    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax

    jmp 0x8:start2_long

bits 64
start2_64:
    push rdi ; magic

    ; identity map first 8MiB and mirror first 8MiB @ KERNEL_START

    SET_PAGE pml4, 0, pml3
    SET_PAGE pml4, KERNEL_START / PML123_SIZE, pml3

    SET_PAGE pml3, 0, pml2

    SET_PAGE pml2, 0, pml1_0
    SET_PAGE pml2, 1, pml1_1
    SET_PAGE pml2, 2, pml1_2
    SET_PAGE pml2, 3, pml1_3

    mov rdi, pml1_0
    mov eax, PAGE_PRESENT | PAGE_WRITE
.loop:
    mov [rdi], eax
    add eax, 0x1000
    add edi, 8
    cmp eax, 0x800000
    jb .loop

    mov rax, 10100000b
    mov cr4, rax

    mov rax, pml4
    mov cr3, rax

    mov rax, cr0
    or eax, 0x80000001 ; pg,pe
    and eax, ~0x10000 ; wp
    mov cr0, rax

    pop rdi ; magic

start2_long:
    ; now load 64-bit gdt with high tss address

    mov rax, tss
    mov rbx, gdt64.tss_entry + 2
    mov [rbx], ax

    shr rax, 16
    mov rbx, gdt64.tss_entry + 4
    mov [rbx], al

    shr rax, 8
    mov rbx, gdt64.tss_entry + 7
    mov [rbx], al

    shr rax, 8
    mov rbx, gdt64.tss_entry + 8
    mov [rbx], eax

    mov rax, gdt64_ptr
    lgdt [rax]

    mov ax, 0x2b ; tss segment in gdt64 + rpl 3
    ltr ax

    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    push 0x10
    push rsp
    pushf
    push 0x08
    push start2_next
    iretq

start2_next:
    ; clear bss
    mov r14, rdi
    mov r15, rsi

    mov rdi, bss
    mov rcx, end
    sub rcx, rdi
    xor al, al
    rep stosb

    mov rsi, r15
    mov rdi, r14

    mov rsp, kernel_stack.bottom
    xor rbp, rbp
    push $
    mov rax, start3
    jmp rax

section .boot.data

align 4
gdt:
    GDT_ENTRY32 0, 0, 0, 0
    GDT_ENTRY32 0, 0, 0x9A, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0x92, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
.end:

align 4
    dw 0
gdt_ptr:
    dw gdt.end - gdt - 1
    dd gdt

section .boot.bss

pml4: resq 512
pml3: resq 512
pml2: resq 512
pml1_0: resq 512
pml1_1: resq 512
pml1_2: resq 512
pml1_3: resq 512

section .data

align 8
gdt64:
    GDT_ENTRY32 0, 0, 0, 0
    GDT_ENTRY32 0, 0, 0x9A, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0x92, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0xFA, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0, 0xF2, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS
.tss_entry:
    GDT_ENTRY32 0, tss.end - tss - 1, 0xE9, 0
    dd 0, 0
.end:

align 8
    times 3 dw 0
gdt64_ptr:
    dw gdt64.end - gdt64 - 1
    dq gdt64

align 8
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
.end:

section .bss

kernel_stack:
    resb 0x1000
.bottom:
