/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * EK-RA8M1 board interface: LEDs and the console on the J-Link virtual COM
 * port (SCI9, 115200 8N1). The application sees only this header.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stddef.h>
#include <stdint.h>

#define BOARD_LED_BLUE      (0u)    /* LED1, P600 */
#define BOARD_LED_GREEN     (1u)    /* LED2, P414 */
#define BOARD_LED_RED       (2u)    /* LED3, P107 */
#define BOARD_LED_COUNT     (3u)

/**
 * @brief Core clock frequency, which also clocks SysTick.
 */
uint32_t board_cpu_hz(void);

void     board_led_set(uint32_t led, int on);
void     board_led_toggle(uint32_t led);

/**
 * @brief Make the calling thread the console's thread: it receives the
 * console's receive notifications. Call it once, from the one thread that
 * reads the console.
 *
 * @return 0 on success, an error number from krt_irq_attach() otherwise.
 */
int      board_console_attach(void);

/**
 * @brief Write @p len bytes, waiting while the transmit FIFO is full.
 */
void     board_console_write(const char *data, size_t len);

/**
 * @brief Write a NUL-terminated string.
 */
void     board_console_puts(const char *s);

/**
 * @brief Wait for one received character and return it.
 */
char     board_console_getc(void);

#endif /* BOARD_H */
