bits 16
org 0x0
%include "kernel/x86inc.asm"

SETUP_SECTS equ 2

start: ; linux_params
    times 0x1f1 db 0
    db SETUP_SECTS ; @0x1f1: setup sectors
    times 12 db 0
    db 0x55, 0xaa ; @0x1fe: boot_flag

start16:
    jmp short .continue ; @0x200: short jump
    dd 0x53726448 ; @0x202: magic
    dw 0x204 ; @0x204: protocol
    times 9 db 0
    db 1 ; @0x211: load high bit
    times 2 db 0
    dd 0x100000 ; @0x214: code32_start
    times 20 db 0
    dd 0x37FFFFFF ; @0x22c: initrd_addr_max
    times 8 db 0
    dd 255 ; @0x238: cmdline_size
    times 48 db 0
.continue:
    jmp .continue2
    times 0x2d0 - ($ - start) db 0
    resb 24 * 20 ; 24 entries should be enough

.continue2:
    xor eax, eax
    xor ebx, ebx

    mov [0x70], eax ; @0x70: rsdp_phy_addr
    mov [0x74], eax

    mov ax, 0xe801
    int 0x15
    jc .check_switch ; failed

    shl ebx, 6
    add eax, ebx
    mov [0x1e0], eax ; @0x1e0 alt_mem_k

    ; read mmap
    xor bp, bp
    xor ebx, ebx
    mov edi, 0x2d0
.e820_loop:
    mov edx, 0x0534D4150
    mov eax, 0xe820
    mov [es:di + 20], dword 1
    mov ecx, 24
    int 0x15
    jc .e820_failed
    inc bp
    add di, 20
    test ebx, ebx
    jne short .e820_loop

.e820_failed:
    mov ax, bp
    mov [0x1e8], al ; @0x1e8 e820_entries

.check_switch:
    mov eax, [0x208] ; @0x208 realmode_swtch
    cmp eax, 0
    jz .switch
    call far [0x208]
    jmp .go_pm

.switch:
    cli
    mov al, 0x80
    out 0x70, al

.go_pm:
    mov eax, [0x214] ; @0x214 code32_start
    mov [.jump32 + 2], eax

    mov esi, ds
    shl esi, 4 ; linux_params

    mov eax, esi
    add eax, gdt - start
    push eax
    push word gdt.end - gdt - 1
    lgdt [esp]

    mov eax, cr0
    or al, 1
    mov cr0, eax

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax

.jump32:
    jmp dword 0x8:0x0

align 4
gdt:
    GDT_ENTRY32 0, 0, 0, 0
    GDT_ENTRY32 0, 0xFFFFFFFF, 0x9A, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0xFFFFFFFF, 0x92, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
.end:

end:
    times (SETUP_SECTS + 1) * 512 - (end - start) db 0
