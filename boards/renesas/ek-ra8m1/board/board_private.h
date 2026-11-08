/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * Device header for the board's own code, shared by startup.c, system.c and
 * board.c. The application uses board.h only.
 *
 * Renesas's R7FA8M1AH.h leaves IRQn_Type to the includer, because the ICU
 * decides which peripheral event each interrupt line carries. This header
 * defines it, with the lines this board routes, and then includes it.
 */

#ifndef BOARD_PRIVATE_H
#define BOARD_PRIVATE_H

#include <stdint.h>

typedef enum IRQn
{
    /* Cortex-M85 exceptions. */
    NonMaskableInt_IRQn     = -14,
    HardFault_IRQn          = -13,
    MemoryManagement_IRQn   = -12,
    BusFault_IRQn           = -11,
    UsageFault_IRQn         = -10,
    SecureFault_IRQn        = -9,
    SVCall_IRQn             = -5,
    DebugMonitor_IRQn       = -4,
    PendSV_IRQn             = -2,
    SysTick_IRQn            = -1,

    /* Peripheral lines; board.c routes an ICU event to each. The number is
     * also what a KideRT thread passes to krt_thread_irqattach(). */
    SCI9_RXI_IRQn           = 0,
    SCI9_ERI_IRQn           = 1,
} IRQn_Type;

#define BOARD_IRQ_COUNT         (2)

#include "R7FA8M1AH.h"

typedef void (*exc_ptr_t)(void);

/* Core clock after SystemInit(): PLL1P, 480 MHz. */
#define BOARD_CPU_HZ            (480000000u)

extern const exc_ptr_t __Vectors[];

void board_sci9_rxi_isr(void);
void board_sci9_eri_isr(void);

/**
 * @brief Busy-wait at least @p us microseconds, timed with SysTick. Only for
 * use before the kernel takes SysTick over.
 */
void system_delay_us(uint32_t us);

#endif /* BOARD_PRIVATE_H */
