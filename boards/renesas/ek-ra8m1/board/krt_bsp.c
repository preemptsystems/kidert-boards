/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * KideRT BSP for the EK-RA8M1: the System Timer on SysTick, the fault hook
 * and the kernel console. krt_bsp_init() is in board/board.c.
 */

#include "krt.h"

#include "board.h"
#include "board_private.h"
#include "krt_bsp.h"

void SysTick_Handler(void);

krt_u32_t       krt_bsp_systimer_hz         = BOARD_CPU_HZ;
const krt_u64_t krt_bsp_systimer_max_period = SysTick_LOAD_RELOAD_Msk;
const krt_s32_t krt_bsp_systimer_irqn       = (krt_s32_t)SysTick_IRQn;


void krt_bsp_systimerreload(krt_u64_t period)
{
    SysTick->CTRL = 0u;
    SysTick->LOAD = (uint32_t)period & SysTick_LOAD_RELOAD_Msk;
    SysTick->VAL  = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}


krt_u64_t krt_bsp_systimergetcount(void)
{
    return SysTick->VAL;
}


/**
 * @brief SysTick interrupt: the System Timer tick.
 */
void SysTick_Handler(void)
{
    krt_systimer_handler();
}


/**
 * @brief Fault hook the kernel calls for every fault it handles. The board
 * records nothing and lets the kernel carry on as it decided.
 */
krt_u32_t krt_bsp_faulthook(krt_u32_t reason, pthread_t thread, krt_u32_t address, krt_u32_t pc, krt_u32_t status)
{
    (void)reason;
    (void)thread;
    (void)address;
    (void)pc;
    (void)status;

    return KRT_FAULT_HOOK_DEFAULT;
}


/**
 * @brief Kernel console output, such as the boot banner, on the board
 * console. Replaces the library's weak no-op.
 */
void krt_console_puts(const char *str)
{
    board_console_puts(str);
}
