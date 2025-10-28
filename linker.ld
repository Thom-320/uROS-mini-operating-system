/* FILE: linker.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x80200000;

    .text : {
        *(.text.start)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }

    . = ALIGN(16);
    _heap_start = .;
    . += 256K;  /* heap for bump allocator */
    _heap_end = .;

    . = ALIGN(16);
    . += 16K;   /* main kernel stack */
    _stack_top = .;
}

