/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * EK-RA8M1 board support: LEDs, and the console on SCI9, which the on-board
 * J-Link brings out as a virtual COM port.
 *
 * The console sends by polling the 16-byte transmit FIFO and receives by
 * interrupt: the receive handler queues the characters and wakes the console
 * thread with krt_thread_notify().
 */

#include <string.h>

#include "krt.h"

#include "board.h"
#include "board_private.h"
#include "krt_bsp.h"

/* Pin function select (PmnPFS) values. */
#define PFS_OUTPUT_LOW          (1u << R_PFS_PORT_PIN_PmnPFS_PDR_Pos)
#define PFS_PERIPHERAL          (1u << R_PFS_PORT_PIN_PmnPFS_PMR_Pos)
#define PFS_DRIVE_HIGH          (3u << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos)
#define PFS_PSEL_SCI_ODD        (5u << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos)  /* SCI1, 3, 5, 7, 9 */
#define PFS_SCI9                (PFS_DRIVE_HIGH | PFS_PERIPHERAL | PFS_PSEL_SCI_ODD)

/* ICU events routed to the console's interrupt lines. */
#define ELC_EVENT_SCI9_RXI      (0x163u)
#define ELC_EVENT_SCI9_ERI      (0x166u)

#define CONSOLE_SCI             R_SCI_B9
#define CONSOLE_MSTPCRB_BIT     (1u << (31u - 9u))  /* SCI9 module stop */
#define CONSOLE_IPL             (12u)
#define CONSOLE_FIFO_DEPTH      (16u)
#define CONSOLE_RX_SIZE         (64u)               /* Power of two */
#define CONSOLE_EVENT_RX        (1u << 0)

/* 115200 baud from the 120 MHz SCICLK: BGDM set (base clock / 16), BRR = 64,
 * 120 MHz / (16 x 65) = 115385 baud, +0.16 %. MDDR at its reset value. */
#define CONSOLE_CCR2            ((128u << 24) | (64u << 8) | (1u << 4))

#define CONSOLE_CFCLR_ALL       ((1u << R_SCI_B0_CFCLR_RDRFC_Pos) | (1u << R_SCI_B0_CFCLR_FERC_Pos) | \
                                 (1u << R_SCI_B0_CFCLR_PERC_Pos)  | (1u << R_SCI_B0_CFCLR_MFFC_Pos) | \
                                 (1u << R_SCI_B0_CFCLR_ORERC_Pos) | (1u << R_SCI_B0_CFCLR_DFERC_Pos) | \
                                 (1u << R_SCI_B0_CFCLR_DPERC_Pos) | (1u << R_SCI_B0_CFCLR_DCMFC_Pos) | \
                                 (1u << R_SCI_B0_CFCLR_ERSC_Pos))
#define CONSOLE_CFCLR_ERRORS    ((1u << R_SCI_B0_CFCLR_ORERC_Pos) | (1u << R_SCI_B0_CFCLR_PERC_Pos) | \
                                 (1u << R_SCI_B0_CFCLR_FERC_Pos))

typedef struct
{
    R_PORT0_Type *port;
    uint32_t      port_number;
    uint32_t      pin;
} board_led_t;

static const board_led_t board_leds[BOARD_LED_COUNT] =
{
    { R_PORT6,  6u,  0u },      /* LED1, blue  */
    { R_PORT4,  4u, 14u },      /* LED2, green */
    { R_PORT1,  1u,  7u },      /* LED3, red   */
};

static uint8_t           board_led_state[BOARD_LED_COUNT];

/* Received characters, from the RX interrupt to the console thread. */
static volatile uint8_t  console_rx[CONSOLE_RX_SIZE];
static volatile uint32_t console_rx_head;
static volatile uint32_t console_rx_tail;


/**
 * @brief Set one pin's function. A peripheral function is selected with the
 * peripheral mode off, then switched on.
 */
static void board_pin_set(uint32_t port, uint32_t pin, uint32_t pfs)
{
    volatile uint32_t *reg = &R_PFS->PORT[port].PIN[pin].PmnPFS;

    if ((pfs & PFS_PERIPHERAL) != 0u)
    {
        *reg &= ~PFS_PERIPHERAL;
        *reg  = pfs & ~PFS_PERIPHERAL;
    }
    *reg = pfs;
}


static void board_pins_init(void)
{
    R_PMISC->PWPRS = 0u;
    R_PMISC->PWPRS = 1u << R_PMISC_PWPRS_PFSWE_Pos;

    for (uint32_t i = 0u; i < BOARD_LED_COUNT; i++)
    {
        board_pin_set(board_leds[i].port_number, board_leds[i].pin, PFS_OUTPUT_LOW);
    }
    board_pin_set(10u, 14u, PFS_SCI9);      /* P1014: TXD9 */
    board_pin_set(10u, 15u, PFS_SCI9);      /* P1015: RXD9 */

    R_PMISC->PWPRS = 0u;
    R_PMISC->PWPRS = 1u << R_PMISC_PWPRS_B0WI_Pos;
}


/**
 * @brief Route a peripheral event to an NVIC line, still disabled.
 */
static void board_irq_route(IRQn_Type irq, uint32_t event)
{
    NVIC_DisableIRQ(irq);
    R_ICU->IELSR[irq]       = event;
    R_ICU->IELSR_b[irq].IR  = 0u;
    NVIC_SetPriority(irq, CONSOLE_IPL);
    NVIC_ClearPendingIRQ(irq);
}


/**
 * @brief Clear a line's ICU request flag, as each handler must.
 */
static void board_irq_ack(IRQn_Type irq)
{
    R_ICU->IELSR_b[irq].IR = 0u;
    (void)R_ICU->IELSR[irq];
}


static void board_console_init(void)
{
    R_MSTP->MSTPCRB &= ~CONSOLE_MSTPCRB_BIT;
    (void)R_MSTP->MSTPCRB;
    system_delay_us(150u);

    board_irq_route(SCI9_RXI_IRQn, ELC_EVENT_SCI9_RXI);
    board_irq_route(SCI9_ERI_IRQn, ELC_EVENT_SCI9_ERI);

    /* 8N1, LSB first, start bit on the falling edge, FIFOs on. */
    CONSOLE_SCI->CCR0 = 1u << R_SCI_B0_CCR0_IDSEL_Pos;
    CONSOLE_SCI->CCR3 = (1u << R_SCI_B0_CCR3_LSBF_Pos) | (2u << R_SCI_B0_CCR3_CHR_Pos) |
                        (1u << R_SCI_B0_CCR3_RXDESEL_Pos) | (1u << R_SCI_B0_CCR3_FM_Pos);
    CONSOLE_SCI->CCR2 = CONSOLE_CCR2;
    CONSOLE_SCI->CCR1 = (1u << R_SCI_B0_CCR1_SPB2DT_Pos) | (1u << R_SCI_B0_CCR1_SPB2IO_Pos);
    CONSOLE_SCI->CCR4 = 0u;

    /* Reset both FIFOs; interrupt on every received byte. */
    CONSOLE_SCI->FCR  = (1u << R_SCI_B0_FCR_TFRST_Pos) | (1u << R_SCI_B0_FCR_RFRST_Pos);
    CONSOLE_SCI->FCR |= (1u << R_SCI_B0_FCR_RTRG_Pos) | ((CONSOLE_FIFO_DEPTH - 1u) << R_SCI_B0_FCR_RSTRG_Pos);
    CONSOLE_SCI->DCR  = 0u;

    CONSOLE_SCI->CFCLR = CONSOLE_CFCLR_ALL;
    CONSOLE_SCI->FFCLR = 1u;

    NVIC_EnableIRQ(SCI9_RXI_IRQn);
    NVIC_EnableIRQ(SCI9_ERI_IRQn);

    CONSOLE_SCI->CCR0 = (1u << R_SCI_B0_CCR0_IDSEL_Pos) | (1u << R_SCI_B0_CCR0_RE_Pos) |
                        (1u << R_SCI_B0_CCR0_RIE_Pos) | (1u << R_SCI_B0_CCR0_TE_Pos);
    while (CONSOLE_SCI->CESR_b.RIST != 1u)
    {
    }
}


/**
 * @brief SCI9 receive interrupt: queue the received characters and wake the
 * console thread.
 */
void board_sci9_rxi_isr(void)
{
    board_irq_ack(SCI9_RXI_IRQn);

    while (CONSOLE_SCI->FRSR_b.R > 0u)
    {
        uint8_t  c    = CONSOLE_SCI->RDR_BY;
        uint32_t head = console_rx_head;

        /* A full buffer drops the character. */
        if ((head - console_rx_tail) < CONSOLE_RX_SIZE)
        {
            console_rx[head & (CONSOLE_RX_SIZE - 1u)] = c;
            console_rx_head = head + 1u;
        }
    }
    CONSOLE_SCI->CFCLR = 1u << R_SCI_B0_CFCLR_RDRFC_Pos;

    krt_thread_notify(CONSOLE_EVENT_RX);
}


/**
 * @brief SCI9 receive error (overrun, framing, parity): drop the character
 * and clear the error so reception continues.
 */
void board_sci9_eri_isr(void)
{
    (void)CONSOLE_SCI->RDR_BY;
    CONSOLE_SCI->CFCLR = CONSOLE_CFCLR_ERRORS;

    board_irq_ack(SCI9_ERI_IRQn);
}


void krt_bsp_init(void)
{
    board_pins_init();
    board_console_init();
}


uint32_t board_cpu_hz(void)
{
    return SystemCoreClock;
}


void board_led_set(uint32_t led, int on)
{
    if (led < BOARD_LED_COUNT)
    {
        uint32_t mask = 1u << board_leds[led].pin;

        board_led_state[led] = (on != 0) ? 1u : 0u;
        /* PCNTR3: set bits in the low half, clear bits in the high half. */
        board_leds[led].port->PCNTR3 = (on != 0) ? mask : (mask << 16);
    }
}


void board_led_toggle(uint32_t led)
{
    if (led < BOARD_LED_COUNT)
    {
        board_led_set(led, board_led_state[led] == 0u);
    }
}


int board_console_attach(void)
{
    return (int)krt_thread_irqattach((krt_u32_t)SCI9_RXI_IRQn);
}


void board_console_write(const char *data, size_t len)
{
    for (size_t i = 0u; i < len; i++)
    {
        while (CONSOLE_SCI->FTSR_b.T >= CONSOLE_FIFO_DEPTH)
        {
        }
        CONSOLE_SCI->TDR_BY = (uint8_t)data[i];
    }
}


void board_console_puts(const char *s)
{
    board_console_write(s, strlen(s));
}


char board_console_getc(void)
{
    uint32_t tail = console_rx_tail;

    while (console_rx_head == tail)
    {
        (void)krt_thread_wait();
    }

    char c = (char)console_rx[tail & (CONSOLE_RX_SIZE - 1u)];
    console_rx_tail = tail + 1u;
    return c;
}
