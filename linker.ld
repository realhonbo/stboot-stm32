ENTRY(Reset_Handler)
_estack = ORIGIN(DTCM) + LENGTH(DTCM);
_Min_Heap_Size  = 0x800;
_Min_Stack_Size = 0x800;

MEMORY
{
/* RAM */
    DTCM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    RAM_D1 (xrw) : ORIGIN = 0x24000000, LENGTH = 512K
    RAM_D2 (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
    RAM_D3 (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
    ITCM   (xrw) : ORIGIN = 0x00001000, LENGTH = 64K /* pre-4KB for Reserved */
    SDRAM  (xrw) : ORIGIN = 0xC0000000, LENGTH = 32M
/* FLASH */
    FLASH   (rx) : ORIGIN = 0x08000000, LENGTH = 2M
    QSPI    (rx) : ORIGIN = 0x90000000, LENGTH = 8M
}

SECTIONS
{
    .isr_vector : {
        . = ALIGN(4);  _isr_start = .;
        KEEP(*(.isr_vector))
    } >ITCM  AT>FLASH
    _isr_size = SIZEOF(.isr_vector);

    .text : {
        . = ALIGN(4);
        *(.text .text* .glue_7 .glue_7t .eh_frame)
        KEEP (*(.init)) KEEP (*(.fini))
        . = ALIGN(4);  _etext = .;
    } >FLASH

    .itcm : {
       . = ALIGN(4);  _itcm_start = .;
       KEEP(*(.itcm))
    } >ITCM  AT>FLASH
    _itcm_at_start = LOADADDR(.itcm);
    _itcm_size = SIZEOF(.itcm);

/* constant data */
    .rodata : {
        . = ALIGN(4); *(.rodata .rodata*) . = ALIGN(4);
    } >FLASH

    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    .preinit_array : {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >FLASH

    .init_array : {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*))) KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >FLASH

    .fini_array : {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*))) KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >FLASH

/* data */
    _sidata = LOADADDR(.data);
    .data : {
        . = ALIGN(4);  _sdata = .;
        *(.data .data*)
    } >DTCM  AT>FLASH

/* commands */
    .shell_cmd : {
        _shell_cmd_start = .;
        KEEP(*(.shell_cmd))
        . = ALIGN(4);  _edata = .;
        _shell_cmd_end = .;
    } >DTCM  AT>FLASH

    . = ALIGN(4);
    .bss : {
        _sbss = .; __bss_start__ = _sbss;
        *(.bss .bss* COMMON)
        . = ALIGN(4); _ebss = .; __bss_end__ = _ebss;
    } >DTCM

    ._user_heap_stack : {
        . = ALIGN(8);
        end = .;  _end = . ;  . = . + _Min_Heap_Size;  . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >DTCM

    /DISCARD/ : {
        libc.a ( * ) libm.a ( * ) libgcc.a ( * )
    }
    .ARM.attributes 0 : { *(.ARM.attributes) }
}

