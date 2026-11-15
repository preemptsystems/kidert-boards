/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * KideRT BSP for the EK-RA8M1: board start-up, the System Timer, which is
 * the Cortex-M SysTick running from the core clock, the fault hook and the
 * kernel console.
 */

#ifndef KRT_BSP_H
#define KRT_BSP_H

#include "krt.h"

/** The clock SysTick counts, in Hz: the 480 MHz core clock. */
extern krt_u32_t        krt_bsp_systimer_hz;

/** Largest value SysTick's 24-bit reload register takes. */
extern const krt_u64_t  krt_bsp_systimer_max_period;

/** SysTick's exception number, -1. */
extern const krt_s32_t  krt_bsp_systimer_irqn;

/**
 * @brief Start a System Timer period of @p period core clock cycles.
 *
 * @param period    Core clock cycles SysTick counts down before it
 *                  interrupts; at most krt_bsp_systimer_max_period.
 */
void        krt_bsp_systimerreload(krt_u64_t period);

/**
 * @brief Cycles left in the current System Timer period.
 */
krt_u64_t   krt_bsp_systimergetcount(void);

/**
 * @brief Set up the board: pins, LEDs and the console. main() calls it
 * once, ahead of krt_kernel_boot().
 */
void        krt_bsp_init(void);

/**
 * @brief Fault hook the kernel calls for every fault it handles.
 *
 * @return KRT_FAULT_HOOK_DEFAULT to carry on as the kernel decided.
 */
krt_u32_t krt_bsp_faulthook(krt_u32_t reason, pthread_t thread, krt_u32_t address, krt_u32_t pc, krt_u32_t status);

/**
 * @brief Kernel console output: the boot banner and diagnostics. Writes
 * @p str to the board console.
 */
void        krt_console_puts(const char *str);

#endif /* KRT_BSP_H */
