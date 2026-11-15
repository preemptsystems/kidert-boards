/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * Linker script for a flat (all-secure) image on the R7FA8M1AH of the
 * EK-RA8M1.
 *
 * Code flash and SRAM are used through their secure aliases. The TCMs, data
 * flash and the option-setting memory are not used; flashing this image
 * leaves the option bytes as they are.
 *
 * SystemInit (board/system.c) copies .data and zeroes .bss, which ends with
 * the kernel heap, and .krt_stackpool.
 */

__STACK_SIZE    = 0x800;
__KRT_HEAP_SIZE = 8K;       /* Kernel heap */
__KRT_POOL_SIZE = 4K;       /* Thread stack pool */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x02000000, LENGTH = 2016K
    RAM   (rwx) : ORIGIN = 0x22000000, LENGTH = 896K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *(.rodata*)
        KEEP(*(.eh_frame*))
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /* Static thread stacks, aligned as KideRT asks. First in RAM so the
     * alignment leaves no gap at the origin. */
    .krt_stackpool (NOLOAD) :
    {
        . = ALIGN(8);
        __krt_stackpool_start__ = .;
        KEEP(*(SORT_BY_ALIGNMENT(.krt_stackpool*)))
        . = ALIGN(8);
        __krt_stackpool_end__ = .;
    } > RAM

    /* Stacks of threads created without one of their own. */
    .krt_pool (NOLOAD) :
    {
        __krt_pool_start__ = .;
        . = . + __KRT_POOL_SIZE;
        __krt_pool_end__ = .;
    } > RAM

    /* Main stack: start-up code, then the interrupt handlers once KideRT runs. */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __StackLimit = .;
        . = . + __STACK_SIZE;
        . = ALIGN(8);
        __StackTop = .;
    } > RAM

    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)

        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);

        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH

    __data_load__ = LOADADDR(.data);

    /* KideRT kernel-local storage. */
    .kls (NOLOAD) :
    {
        __kls_base = .;
        KEEP(*(.kls))
        KEEP(*(.kls.*))
    } > RAM

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        __krt_heap_start__ = .;
        . = . + __KRT_HEAP_SIZE;
        __krt_heap_end__ = .;
        __bss_end__ = .;
    } > RAM

    PROVIDE(end = .);
}
