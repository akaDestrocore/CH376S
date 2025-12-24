/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file           ch376s_uart.h
 * @brief          CH376S UART hardware interface definitions
 * @details        Hardware abstraction for 8-bit UART communication
 */

#ifndef CH376S_UART_H
#define CH376S_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "ch376s.h"

/**
 * Platform-specific includes
 */
#if defined(CONFIG_SOC_RP2350A_M33) || defined(CONFIG_SOC_RP2040) || defined(CONFIG_SOC_SERIES_RP2XXX)
    #include <hardware/pio.h>
    #include <hardware/clocks.h>
    #include <hardware/gpio.h>
#else
    #error "CH376S currently only supported on RP2040/RP2350"
#endif

/**
 * Hardware context structure
 */
#if defined(CONFIG_SOC_RP2350A_M33) || defined(CONFIG_SOC_RP2040) || defined(CONFIG_SOC_SERIES_RP2XXX)
    #define CH376S_A_USART_INDEX 0
    #define CH376S_B_USART_INDEX 1   

    /* Pin definitions - you can customize these */
    #define PIO_UART_TX_PIN_CH376S_A 12
    #define PIO_UART_RX_PIN_CH376S_A 13
    #define PIO_UART_TX_PIN_CH376S_B 14
    #define PIO_UART_RX_PIN_CH376S_B 15

    #define PIO_UART_SM_TX 0
    #define PIO_UART_SM_RX 1

    typedef struct {
        const char *name;
        uint32_t baudrate;
        struct gpio_dt_spec int_gpio;
        PIO pio;
        uint sm_tx;
        uint sm_rx;
        uint tx_pin;
        uint rx_pin;
        uint offset_tx;
        uint offset_rx;
    } ch376s_HwContext_t;

#endif

// Abstract wrapper for CH376S hardware initialization
int ch376s_hwInitManual(const char *name, int uart_index, 
                        const struct gpio_dt_spec *int_gpio, 
                        uint32_t initial_baudrate, 
                        struct ch376s_Context_t **ppCtxOut);

// Abstract wrapper for CH376S baudrate modification
int ch376s_hwSetBaudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate);


#ifdef __cplusplus
}
#endif

#endif /* CH376S_UART_H */