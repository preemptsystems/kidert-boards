/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * System initialization for the R7FA8M1AH, called by Reset_Handler before
 * main(): caches, C runtime, clocks, I/O voltage and the security attribution
 * of a flat image (everything secure).
 *
 * Clock tree: 20 MHz crystal -> PLL1 960 MHz -> PLL1P 480 MHz for the CPU,
 * ICLK 240 MHz, PCLKA 120 MHz, PCLKB 60 MHz, and SCICLK 120 MHz for the
 * console. The register sequences follow those of Renesas's FSP v6.6.0 for
 * this clock tree.
 */

#include "board_private.h"

#define SYSTEM_MOCO_HZ          (8000000u)          /* Core clock out of reset */

#define SYSTEM_PRCR_KEY         (0xA5u << R_SYSTEM_PRCR_PRKEY_Pos)
#define SYSTEM_PRCR_CGC         (1u << R_SYSTEM_PRCR_PRC0_Pos)  /* Clock generation */
#define SYSTEM_PRCR_LPC         (1u << R_SYSTEM_PRCR_PRC1_Pos)  /* Low power, I/O voltage */
#define SYSTEM_PRCR_SAR         (1u << R_SYSTEM_PRCR_PRC4_Pos)  /* Security attribution */
#define SYSTEM_SRAMPRCR_KEY     (0xA5u << 8)
#define SYSTEM_SRAMPRCR_SRAMWTSC (1u << 0)

/* Divider field values in SCKDIVCR and SCKDIVCR2. */
#define SYSTEM_DIV_1            (0u)
#define SYSTEM_DIV_2            (1u)
#define SYSTEM_DIV_4            (2u)
#define SYSTEM_DIV_8            (3u)

#define SYSTEM_SOURCE_PLL1P     (5u)                /* SCKSCR and SCICKCR source */
#define SYSTEM_MOMCR_20MHZ      (3u << R_SYSTEM_MOMCR_MODRV0_Pos)   /* Crystal, drive for 20 MHz */
#define SYSTEM_MOSC_WAIT        (9u)
#define SYSTEM_FLASH_WAIT       (4u)                /* For ICLK up to 240 MHz */
#define SYSTEM_SRAM_WAIT        (1u)                /* For ICLK above 120 MHz */
#define SYSTEM_SETTLE_US        (150u)
#define SYSTEM_PMSAR_COUNT      (12u)

extern uint32_t __data_start__[];
extern uint32_t __data_end__[];
extern uint32_t __data_load__[];
extern uint32_t __bss_start__[];
extern uint32_t __bss_end__[];
extern uint32_t __krt_stackpool_start__[];
extern uint32_t __krt_stackpool_end__[];
extern uint32_t __StackLimit[];
extern void   (*__init_array_start[])(void);
extern void   (*__init_array_end[])(void);

uint32_t SystemCoreClock = SYSTEM_MOCO_HZ;

static void system_runtime_init(void);
static void system_clock_init(void);
static void system_sciclk_init(void);
static void system_io_init(void);
static void system_security_init(void);


void system_delay_us(uint32_t us)
{
    uint32_t cycles = (SystemCoreClock / 1000000u) * us;

    SysTick->CTRL = 0u;
    SysTick->LOAD = (cycles > SysTick_LOAD_RELOAD_Msk) ? SysTick_LOAD_RELOAD_Msk : cycles;
    SysTick->VAL  = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0u)
    {
    }
    SysTick->CTRL = 0u;
}


/**
 * @brief Copy .data and zero .bss and the KideRT stack pool.
 */
static void system_runtime_init(void)
{
    uint32_t       *dst = __data_start__;
    const uint32_t *src = __data_load__;

    while (dst < __data_end__)
    {
        *dst++ = *src++;
    }
    for (dst = __bss_start__; dst < __bss_end__; dst++)
    {
        *dst = 0u;
    }
    for (dst = __krt_stackpool_start__; dst < __krt_stackpool_end__; dst++)
    {
        *dst = 0u;
    }
}


/**
 * @brief Main oscillator, PLL1 and the switch to the 480 MHz core clock.
 */
static void system_clock_init(void)
{
    R_SYSTEM->PRCR = (uint16_t)(SYSTEM_PRCR_KEY | SYSTEM_PRCR_CGC | SYSTEM_PRCR_LPC);

    /* Flash cache: invalidate, then enable. */
    R_FCACHE->FCACHEIV = 1u;
    while (R_FCACHE->FCACHEIV != 0u)
    {
    }
    R_FCACHE->FCACHEE = 1u;

    /* Main oscillator: the 20 MHz crystal. */
    R_SYSTEM->MOMCR    = (uint8_t)SYSTEM_MOMCR_20MHZ;
    R_SYSTEM->MOSCWTCR = (uint8_t)SYSTEM_MOSC_WAIT;
    R_SYSTEM->MOSCCR   = 0u;
    while (R_SYSTEM->OSCSF_b.MOSCSF != 1u)
    {
    }

    /* PLL1: 20 MHz / 2 x 96 = 960 MHz; outputs P, Q and R divide it by 2. */
    R_SYSTEM->PLLCCR  = (uint16_t)((95u << R_SYSTEM_PLLCCR_PLLMUL_Pos) |
                                   (0u  << R_SYSTEM_PLLCCR_PLLMULNF_Pos) |
                                   (0u  << R_SYSTEM_PLLCCR_PLSRCSEL_Pos) |
                                   (1u  << R_SYSTEM_PLLCCR_PLIDIV_Pos));
    R_SYSTEM->PLLCCR2 = (uint16_t)((1u << R_SYSTEM_PLLCCR2_PLODIVR_Pos) |
                                   (1u << R_SYSTEM_PLLCCR2_PLODIVQ_Pos) |
                                   (1u << R_SYSTEM_PLLCCR2_PLODIVP_Pos));
    R_SYSTEM->PLLCR   = 0u;
    while (R_SYSTEM->OSCSF_b.PLLSF != 1u)
    {
    }

    /* Memory wait states for the faster clocks, before switching. */
    R_FCACHE->FLWT = (uint8_t)SYSTEM_FLASH_WAIT;
    R_SRAM->SRAMPRCR = SYSTEM_SRAMPRCR_KEY | SYSTEM_SRAMPRCR_SRAMWTSC;
    __DMB();
    R_SRAM->SRAMWTSC = SYSTEM_SRAM_WAIT;
    __DMB();
    R_SRAM->SRAMPRCR = SYSTEM_SRAMPRCR_KEY;

    /* Switch to PLL1P with the CPU at half speed first, then full speed, to
     * keep the current step small. */
    R_SYSTEM->SCKDIVCR = (SYSTEM_DIV_8 << R_SYSTEM_SCKDIVCR_FCK_Pos)  |     /* FCLK   60 MHz */
                         (SYSTEM_DIV_2 << R_SYSTEM_SCKDIVCR_ICK_Pos)  |     /* ICLK  240 MHz */
                         (SYSTEM_DIV_2 << R_SYSTEM_SCKDIVCR_PCKE_Pos) |     /* PCLKE 240 MHz */
                         (SYSTEM_DIV_4 << R_SYSTEM_SCKDIVCR_BCK_Pos)  |     /* BCLK  120 MHz */
                         (SYSTEM_DIV_4 << R_SYSTEM_SCKDIVCR_PCKA_Pos) |     /* PCLKA 120 MHz */
                         (SYSTEM_DIV_8 << R_SYSTEM_SCKDIVCR_PCKB_Pos) |     /* PCLKB  60 MHz */
                         (SYSTEM_DIV_8 << R_SYSTEM_SCKDIVCR_PCKC_Pos) |     /* PCLKC  60 MHz */
                         (SYSTEM_DIV_4 << R_SYSTEM_SCKDIVCR_PCKD_Pos);      /* PCLKD 120 MHz */
    R_SYSTEM->SCKDIVCR2 = (uint8_t)SYSTEM_DIV_2;
    R_SYSTEM->SCKSCR    = (uint8_t)SYSTEM_SOURCE_PLL1P;
    SystemCoreClock     = BOARD_CPU_HZ / 2u;
    system_delay_us(SYSTEM_SETTLE_US);

    R_SYSTEM->SCKDIVCR2 = (uint8_t)SYSTEM_DIV_1;
    SystemCoreClock     = BOARD_CPU_HZ;
    system_delay_us(SYSTEM_SETTLE_US);

    system_sciclk_init();

    R_SYSTEM->PRCR = (uint16_t)SYSTEM_PRCR_KEY;
}


/**
 * @brief SCICLK for the SCI_B console: PLL1P / 4 = 120 MHz.
 */
static void system_sciclk_init(void)
{
    R_SYSTEM->SCICKCR |= (uint8_t)(1u << R_SYSTEM_SCICKCR_CKSREQ_Pos);
    while ((R_SYSTEM->SCICKCR & (1u << R_SYSTEM_SCICKCR_CKSRDY_Pos)) == 0u)
    {
    }
    R_SYSTEM->SCICKDIVCR = (uint8_t)SYSTEM_DIV_4;
    R_SYSTEM->SCICKCR    = (uint8_t)((SYSTEM_SOURCE_PLL1P << R_SYSTEM_SCICKCR_SCICKSEL_Pos) |
                                     (1u << R_SYSTEM_SCICKCR_CKSREQ_Pos) |
                                     (1u << R_SYSTEM_SCICKCR_CKSRDY_Pos));
    R_SYSTEM->SCICKCR   &= (uint8_t)~(1u << R_SYSTEM_SCICKCR_CKSREQ_Pos);
    while ((R_SYSTEM->SCICKCR & (1u << R_SYSTEM_SCICKCR_CKSRDY_Pos)) != 0u)
    {
    }
}


/**
 * @brief I/O ports at 3.3 V.
 */
static void system_io_init(void)
{
    R_SYSTEM->PRCR  = (uint16_t)(SYSTEM_PRCR_KEY | SYSTEM_PRCR_LPC);
    R_SYSTEM->LVOCR = 0u;
    R_SYSTEM->PRCR  = (uint16_t)SYSTEM_PRCR_KEY;
}


/**
 * @brief Flat image: every pin and peripheral belongs to the secure world.
 */
static void system_security_init(void)
{
    R_SYSTEM->PRCR = (uint16_t)(SYSTEM_PRCR_KEY | SYSTEM_PRCR_SAR);
    for (uint32_t i = 0u; i < SYSTEM_PMSAR_COUNT; i++)
    {
        R_PMISC->PMSAR[i].PMSAR = 0u;
    }
    R_PSCU->PSARB = 0u;
    R_PSCU->PSARC = 0u;
    R_PSCU->PSARD = 0u;
    R_PSCU->PSARE = 0u;
    R_SYSTEM->PRCR = (uint16_t)SYSTEM_PRCR_KEY;
}


void SystemInit(void)
{
    /* Instruction cache, branch prediction and the low-overhead-branch cache. */
    SCB_EnableICache();
    SCB->CCR |= SCB_CCR_BP_Msk | SCB_CCR_LOB_Msk;
    __DSB();
    __ISB();

    SCB->VTOR = (uint32_t)__Vectors;
    __set_MSPLIM((uint32_t)__StackLimit);

    system_runtime_init();
    system_clock_init();
    system_io_init();
    system_security_init();

    for (void (**ctor)(void) = __init_array_start; ctor < __init_array_end; ctor++)
    {
        (*ctor)();
    }
}


void SystemCoreClockUpdate(void)
{
    /* SystemCoreClock is kept current by system_clock_init(). */
}
