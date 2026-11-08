/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * KideRT BSP for the EK-RA8M1: board start-up and the System Timer, which is
 * the Cortex-M SysTick running from the core clock.
 */

#ifndef KRT_BSP_H
#define KRT_BSP_H

#include "krt.h"

/** SysTick's exception number, for krt_systimer_configure(). */
#define KRT_BSP_SYSTIMER_IRQN    (-1)

/**
 * @brief Set up the board: pins, LEDs and the console. Call once from
 * main(), before krt_systimer_configure() and krt_kernel_boot().
 */
void      krt_bsp_init(void);

/**
 * @brief Start a System Timer period of @p period core clock cycles.
 *
 * @param period    Cycles until the next System Timer interrupt; at most
 *                  0xFFFFFF.
 */
void      krt_bsp_systimerreload(krt_u64_t period);

/**
 * @brief Cycles left in the current System Timer period.
 */
krt_u64_t krt_bsp_systimergetcount(void);

/**
 * @brief Fault hook the kernel calls for every fault it handles.
 *
 * @return KRT_FAULT_HOOK_DEFAULT to carry on as the kernel decided.
 */
krt_u32_t krt_bsp_faulthook(krt_u32_t reason, pthread_t thread, krt_u32_t address, krt_u32_t pc, krt_u32_t status);

#endif /* KRT_BSP_H */
