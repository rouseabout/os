ENTRY(start)

KERNEL_ADDRESS = DEFINED(ARCH_i686) ? 0xC0000000 : 0x8000000000;

SECTIONS {
    . = 0x100000;

    .boot.header : { *(.boot.header) }
    .boot.text : { *(.boot.text) }
    .boot.data : { *(.boot.data) }
    .boot.bss ALIGN(4096) : { *(.boot.bss) }

    boot_end = .;

    . += KERNEL_ADDRESS;

    text = .;

    .text ALIGN(4096) : AT(ADDR(.text) - KERNEL_ADDRESS) {
        *(.text)
    }

    .data ALIGN(4096) : AT(ADDR(.data) - KERNEL_ADDRESS) {
        *(.data)
        *(.rodata)
        *(.rodata.*)
    }

    .eh_frame_hdr : {
        *(.eh_frame_hdr)
    }

    .eh_frame : {
        *(.eh_frame)
        *(.eh_frame.*)
    }

    bss = .;

    .bss ALIGN(4096) : AT(ADDR(.bss) - KERNEL_ADDRESS) {
        *(.bss)
    }

    end = .;

    /DISCARD/ : {
        *(.comment)
    }
}
