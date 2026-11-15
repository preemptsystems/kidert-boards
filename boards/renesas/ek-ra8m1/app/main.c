/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Preempt Systems Oy */

/*
 * EK-RA8M1 application: KideRT with three threads.
 *
 * - Blink: toggles the blue LED every 500 ms.
 * - Breathe: fades the green LED in and out with 1 kHz software PWM. Every
 *   1 ms period is an on-delay and an off-delay that add up to 1 ms; the
 *   on-delay rises from 0 to 1 ms in 100 steps of 10 periods each, stays on
 *   for 300 ms and falls back.
 * - Console: prints a banner on the J-Link virtual COM port and echoes what
 *   it receives, woken by the UART receive interrupt through
 *   krt_irq_notify().
 */

#include <pthread.h>
#include <time.h>

#include "krt.h"
#include "krt_bsp.h"

#include "board.h"

#define APP_STACK_SIZE          (2048u)
#define APP_BREATHE_STACK_SIZE  (512u)
#define APP_BLINK_PERIOD_NS     (500000000L)
#define APP_PWM_PERIOD_NS       (1000000L)      /* 1 kHz */
#define APP_PWM_STEPS           (100L)          /* Brightness steps, each way */
#define APP_PWM_STEP_PERIODS    (10L)           /* Periods per step: a 1 s fade */
#define APP_PWM_HOLD_NS         (300000000L)    /* At full brightness */

/* Each stack in a section of its own, so the linker can sort them by
 * alignment and leave no gaps. */
static krt_u8_t app_blink_stack[APP_STACK_SIZE]           __attribute__((aligned(APP_STACK_SIZE))) __attribute__((section(".krt_stackpool.blink")));
static krt_u8_t app_breathe_stack[APP_BREATHE_STACK_SIZE] __attribute__((aligned(APP_BREATHE_STACK_SIZE))) __attribute__((section(".krt_stackpool.breathe")));
static krt_u8_t app_console_stack[APP_STACK_SIZE]         __attribute__((aligned(APP_STACK_SIZE))) __attribute__((section(".krt_stackpool.console")));

static pthread_t app_blink_handle;
static pthread_t app_breathe_handle;
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
 * @brief Fade the green LED in and out with software PWM.
 *
 * The on-time grows with the square of the step, which the eye sees as an
 * even change in brightness.
 *
 * @param argument Unused.
 * @return Never returns.
 */
static void *app_breathe_thread(void *argument)
{
    const struct timespec hold  = { .tv_sec = 0, .tv_nsec = APP_PWM_HOLD_NS };
    struct timespec       delay = { .tv_sec = 0, .tv_nsec = 0 };
    long            step  = 0;
    long            dir   = 1;

    (void)argument;

    for (;;)
    {
        long on_ns = (APP_PWM_PERIOD_NS / APP_PWM_STEPS) * step * step / APP_PWM_STEPS;

        for (long period = 0; period < APP_PWM_STEP_PERIODS; period++)
        {
            if (on_ns > 0)
            {
                board_led_set(BOARD_LED_GREEN, 1);
                delay.tv_nsec = on_ns;
                (void)nanosleep(&delay, NULL);
            }
            if (on_ns < APP_PWM_PERIOD_NS)
            {
                board_led_set(BOARD_LED_GREEN, 0);
                delay.tv_nsec = APP_PWM_PERIOD_NS - on_ns;
                (void)nanosleep(&delay, NULL);
            }
        }

        step += dir;
        if (step == APP_PWM_STEPS)
        {
            /* Stay at full brightness for a moment before fading out. */
            board_led_set(BOARD_LED_GREEN, 1);
            (void)nanosleep(&hold, NULL);
            dir = -1;
        }
        else if (step == 0)
        {
            dir = 1;
        }
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

    pthread_attr_setstack(&attr, app_breathe_stack, sizeof(app_breathe_stack));
    if (pthread_create(&app_breathe_handle, &attr, app_breathe_thread, NULL) != 0)
    {
        return "Failed to create app_breathe_thread";
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

    return krt_kernel_boot(NULL, NULL, app_startup_thread, NULL);
}
