ENTRY(start)

SECTIONS {
    . = 0x100000;

    .text ALIGN(4096) : {
        *(.text)
    }

    .data ALIGN(4096) : {
        *(.data)
        *(.rodata)
    }

    bss = .;

    .bss ALIGN(4096) : {
        *(.bss)
    }

    end = .;

    /DISCARD/ : {
        *(.comment)
    }
}
