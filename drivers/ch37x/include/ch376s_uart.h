/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                          GhostHIDe Project                            ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 * 
 * @file           ch376s_uart.h
 * @brief          CH376S UART hardware interface definitions (8-bit mode)
 * 
 * @author         destrocore
 * @date           2025
 * 
 * @details
 * Hardware abstraction layer for CH376S communication via standard 8-bit UART.
 * Unlike CH375, CH376S doesn't require 9th bit for command/data differentiation.
 * 
 * @copyright 
 * Copyright (c) 2025 akaDestrocore
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
 * Platform-specific includes - CH376S only supports RP2040/RP2350 for now
 */
#if defined(CONFIG_SOC_RP2350A_M33) || defined(CONFIG_SOC_RP2040) || defined(CONFIG_SOC_SERIES_RP2XXX)
    #include <hardware/pio.h>
    #include <hardware/clocks.h>
    #include <hardware/gpio.h>
#else
    #error "CH376S currently only supports RP2040/RP2350 platforms"
#endif

/**
 * Hardware context structure for CH376S (RP2 with 8-bit PIO UART)
 */
#define CH376S_A_USART_INDEX 0
#define CH376S_B_USART_INDEX 1

#define PIO_UART_TX_PIN_CH376S_A 4
#define PIO_UART_RX_PIN_CH376S_A 5
#define PIO_UART_TX_PIN_CH376S_B 8
#define PIO_UART_RX_PIN_CH376S_B 9

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

/**
 * @brief Initialize CH376S hardware layer
 * @param name Device name for logging
 * @param uart_index UART index (0 or 1)
 * @param int_gpio INT GPIO pin (NULL for polling)
 * @param initial_baudrate Initial baudrate
 * @param ppCtxOut Output context pointer
 * @return 0 on success, negative error code otherwise
 */
int ch376s_hwInitManual(const char *name, int uart_index,
                         const struct gpio_dt_spec *int_gpio,
                         uint32_t initial_baudrate,
                         struct ch376s_Context_t **ppCtxOut);

/**
 * @brief Set CH376S baudrate
 * @param pCtx CH376S context
 * @param baudrate New baudrate
 * @return 0 on success, negative error code otherwise
 */
int ch376s_hwSetBaudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate);

#ifdef __cplusplus
}
#endif

#endif /* CH376S_UART_H */
