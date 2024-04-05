ENTRY(start)

SECTIONS {

    . = 0x10000000;

    .text ALIGN(4096) : {
        *(.text)
    }

    .data ALIGN(4096) : {
        *(.data)
        *(.rodata)
    }

    .bss ALIGN(4096) : {
        *(.bss)
    }

    end = .;

    /DISCARD/ : {
        *(.comment)
    }
}
