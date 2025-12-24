/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                          GhostHIDe Project                            ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 * 
 * @file           ch376s_uart.c
 * @brief          CH376S UART abstraction layer (8-bit mode)
 * 
 * @author         destrocore
 * @date           2025
 * 
 * @details
 * Platform-independent UART wrapper for CH376S. Currently only RP2 platforms
 * are supported with 8-bit PIO UART.
 * 
 * @copyright 
 * Copyright (c) 2025 akaDestrocore
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ch376s_uart.h"

LOG_MODULE_REGISTER(ch376s_uart, LOG_LEVEL_DBG);

#if defined(CONFIG_SOC_RP2350A_M33) || defined(CONFIG_SOC_RP2040) || defined(CONFIG_SOC_SERIES_RP2XXX)
    extern int ch376s_rp2_hw_init(const char *name, int uart_index,
                                    const struct gpio_dt_spec *int_gpio,
                                    uint32_t initial_baudrate,
                                    struct ch376s_Context_t **ppCtxOut);
    extern int ch376s_rp2_set_baudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate);
#else
    #error "CH376S only supports RP2040/RP2350 platforms"
#endif

/**
 * @brief Wrapper for CH376S hardware initialization
 */
int ch376s_hwInitManual(const char *name, int uart_index,
                         const struct gpio_dt_spec *int_gpio,
                         uint32_t initial_baudrate,
                         struct ch376s_Context_t **ppCtxOut) {
#if defined(CONFIG_SOC_RP2350A_M33)
    LOG_INF("Platform: RP2350 (RPI Pico 2) - CH376S 8-bit UART");
    return ch376s_rp2_hw_init(name, uart_index, int_gpio, initial_baudrate, ppCtxOut);
#elif defined(CONFIG_SOC_RP2040)
    LOG_INF("Platform: RP2040 (RPI Pico) - CH376S 8-bit UART");
    return ch376s_rp2_hw_init(name, uart_index, int_gpio, initial_baudrate, ppCtxOut);
#else
    LOG_ERR("ERROR: CH376S only supported on RP2040/RP2350!");
    return -ENOTSUP;
#endif
}

/**
 * @brief Set baudrate
 */
int ch376s_hwSetBaudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate) {
    LOG_INF("ch376s_hwSetBaudrate called: baud=%u", baudrate);

#if defined(CONFIG_SOC_RP2350A_M33) || defined(CONFIG_SOC_RP2040) || defined(CONFIG_SOC_SERIES_RP2XXX)
    return ch376s_rp2_set_baudrate(pCtx, baudrate);
#else
    LOG_ERR("ERROR: No platform defined!");
    return -ENOTSUP;
#endif
}
