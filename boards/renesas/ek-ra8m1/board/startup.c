/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * Reset entry and vector table.
 *
 * KideRT defines the SVC, PendSV and fault handlers. They are declared here
 * with no default of their own.
 */

#include "board_private.h"

#define VECTOR_COUNT    (16 + BOARD_IRQ_COUNT)

extern uint32_t __StackTop[];

void Reset_Handler(void);
void Default_Handler(void);
int  main(void);

/* KideRT kernel (libkrt.a). */
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);

/* KideRT BSP (board/krt_bsp.c). */
void SysTick_Handler(void);

/* KideRT defines it when it runs in the Secure state, as it does here. */
void SecureFault_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* Not used on this board. */
void NMI_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)    __attribute__((weak, alias("Default_Handler")));

const exc_ptr_t __Vectors[VECTOR_COUNT] __attribute__((section(".vectors"), used)) =
{
    (exc_ptr_t)__StackTop,      /* Initial stack pointer */
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    SecureFault_Handler,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    [16 + SCI9_RXI_IRQn] = board_sci9_rxi_isr,
    [16 + SCI9_ERI_IRQn] = board_sci9_eri_isr,
};


/**
 * @brief Reset entry: system initialization, then the application.
 */
void Reset_Handler(void)
{
    SystemInit();

    (void)main();

    for (;;)
    {
        /* main() returns only if the kernel could not boot. */
    }
}


/**
 * @brief Exceptions nothing else handles stop here.
 */
void Default_Handler(void)
{
    for (;;)
    {
    }
}
