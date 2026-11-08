/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * EK-RA8M1 application: KideRT with two threads. One blinks the blue LED every
 * 500 ms; the other prints a banner on the J-Link virtual COM port and echoes
 * what it receives, woken by the UART interrupts through
 * krt_thread_notify().
 */

#include <pthread.h>
#include <time.h>

#include "krt.h"
#include "krt_bsp.h"

#include "board.h"

#define APP_SYSTICK_MAX_PERIOD  (0xFFFFFFu)     /* 24-bit SysTick */
#define APP_STACK_SIZE          (2048u)
#define APP_BLINK_PERIOD_NS     (500000000L)

static krt_u8_t app_blink_stack[APP_STACK_SIZE]   __attribute__((aligned(APP_STACK_SIZE))) __attribute__((section(".krt_stackpool")));
static krt_u8_t app_console_stack[APP_STACK_SIZE] __attribute__((aligned(APP_STACK_SIZE))) __attribute__((section(".krt_stackpool")));

static pthread_t app_blink_handle;
static pthread_t app_console_handle;


/**
 * @brief Blink the blue LED.
 *
 * @param argument Unused.
 * @return Never returns.
 */
static void *app_blink_thread(void *argument)
{
    const struct timespec period = { .tv_sec = 0, .tv_nsec = APP_BLINK_PERIOD_NS };

    (void)argument;

    for (;;)
    {
        board_led_toggle(BOARD_LED_BLUE);
        (void)nanosleep(&period, NULL);
    }

    return NULL;
}


/**
 * @brief Print the banner, then echo the console input.
 *
 * @param argument Unused.
 * @return Error message, if the console cannot be attached.
 */
static void *app_console_thread(void *argument)
{
    (void)argument;

    if (board_console_attach() != 0)
    {
        board_led_set(BOARD_LED_RED, 1);
        return "Failed to attach to the console interrupts";
    }

    board_console_puts("\r\nKideRT on EK-RA8M1 (R7FA8M1AH, Cortex-M85)\r\n"
                       "Type and the board echoes it back.\r\n");

    for (;;)
    {
        char c = board_console_getc();

        if (c == '\r')
        {
            board_console_puts("\r\n");
        }
        else
        {
            board_console_write(&c, 1u);
        }
        board_led_toggle(BOARD_LED_GREEN);
    }

    return NULL;
}


/**
 * @brief First thread: create the application threads.
 *
 * @param argument Unused.
 * @return Error message, or NULL.
 */
static void *app_startup_thread(void *argument)
{
    pthread_attr_t attr;

    (void)argument;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, app_blink_stack, sizeof(app_blink_stack));
    if (pthread_create(&app_blink_handle, &attr, app_blink_thread, NULL) != 0)
    {
        return "Failed to create app_blink_thread";
    }

    pthread_attr_setstack(&attr, app_console_stack, sizeof(app_console_stack));
    if (pthread_create(&app_console_handle, &attr, app_console_thread, NULL) != 0)
    {
        return "Failed to create app_console_thread";
    }

    pthread_attr_destroy(&attr);
    return NULL;
}


int main(void)
{
    krt_bsp_init();

    /* SysTick runs from the core clock SystemInit() has just set. */
    if (krt_systimer_configure(krt_bsp_systickreload,
                               krt_bsp_systickgetcount,
                               APP_SYSTICK_MAX_PERIOD,
                               (krt_s32_t)board_cpu_hz(),
                               KRT_BSP_SYSTICK_IRQN) != 0)
    {
        board_led_set(BOARD_LED_RED, 1);
        return -1;
    }

    return krt_kernel_boot(NULL, NULL, app_startup_thread, NULL);
}
