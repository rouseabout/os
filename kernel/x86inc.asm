GRAN_64_BIT_MODE equ (1<<5)
GRAN_32_BIT_MODE equ (1<<6)
GRAN_4KIB_BLOCKS equ (1<<7)

%macro GDT_ENTRY32 4 ; base, limit, access, granularity
    dw (%2 & 0xFFFF)            ; limit_low
    dw (%1 & 0xFFFF)            ; base_low
    db (%1 >> 16) & 0xFF        ; base_middle
    db %3                       ; access
    db ((%2 >> 16) & 0xF) | %4  ; limit_high|granularity
    db (%1 >> 24) & 0xFF        ; base_high
%endmacro

PAGE_PRESENT equ (1<<0)
PAGE_WRITE equ (1<<1)

%macro HAS_CPUID 0
    pushfd
    pushfd
    or dword [esp], 0x200000
    popfd
    pushfd
    pop eax
    xor eax, [esp]
    popfd
    and eax, 0x200000
%endmacro
