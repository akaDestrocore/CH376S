/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                          GhostHIDe Project                            ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 * 
 * @file           ch37x_common.h
 * @brief          Common API wrapper for CH375/CH376S USB host controllers
 * 
 * @author         destrocore
 * @date           2025
 * 
 * @details
 * Provides unified API that abstracts differences between CH375 (9-bit UART)
 * and CH376S (8-bit UART) implementations. Compile-time selection via
 * USE_CH376S preprocessor flag. ALL constants, macros, and functions are
 * unified under the ch37x_ namespace.
 * 
 * @copyright 
 * Copyright (c) 2025 akaDestrocore
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CH37X_COMMON_H
#define CH37X_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>
#include <stdbool.h>

/* Include chip-specific headers */
#ifdef USE_CH376S
    #include "ch376s.h"
    #include "ch376s_uart.h"
    typedef struct ch376s_Context_t ch37x_Context_t;
#else
    #include "ch375.h"
    #include "ch375_uart.h"
    typedef struct ch375_Context_t ch37x_Context_t;
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - Baudrates
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_DEFAULT_BAUDRATE      CH376S_DEFAULT_BAUDRATE
    #define CH37X_WORK_BAUDRATE         CH376S_WORK_BAUDRATE
#else
    #define CH37X_DEFAULT_BAUDRATE      CH375_DEFAULT_BAUDRATE
    #define CH37X_WORK_BAUDRATE         CH375_WORK_BAUDRATE
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - UART Indices
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_A_USART_INDEX         CH376S_A_USART_INDEX
    #define CH37X_B_USART_INDEX         CH376S_B_USART_INDEX
#else
    #define CH37X_A_USART_INDEX         CH375_A_USART_INDEX
    #define CH37X_B_USART_INDEX         CH375_B_USART_INDEX
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - Error Codes
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_SUCCESS               CH376S_SUCCESS
    #define CH37X_ERROR                 CH376S_ERROR
    #define CH37X_PARAM_INVALID         CH376S_PARAM_INVALID
    #define CH37X_WRITE_CMD_FAILED      CH376S_WRITE_CMD_FAILED
    #define CH37X_READ_DATA_FAILED      CH376S_READ_DATA_FAILED
    #define CH37X_NO_EXIST              CH376S_NO_EXIST
    #define CH37X_TIMEOUT               CH376S_TIMEOUT
    #define CH37X_NOT_FOUND             CH376S_NOT_FOUND
#else
    #define CH37X_SUCCESS               CH375_SUCCESS
    #define CH37X_ERROR                 CH375_ERROR
    #define CH37X_PARAM_INVALID         CH375_PARAM_INVALID
    #define CH37X_WRITE_CMD_FAILED      CH375_WRITE_CMD_FAILED
    #define CH37X_READ_DATA_FAILED      CH375_READ_DATA_FAILED
    #define CH37X_NO_EXIST              CH375_NO_EXIST
    #define CH37X_TIMEOUT               CH375_TIMEOUT
    #define CH37X_NOT_FOUND             CH375_NOT_FOUND
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - USB Modes
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_USB_MODE_INVALID      CH376S_USB_MODE_INVALID
    #define CH37X_USB_MODE_NO_SOF       CH376S_USB_MODE_NO_SOF
    #define CH37X_USB_MODE_SOF_AUTO     CH376S_USB_MODE_SOF_AUTO
    #define CH37X_USB_MODE_RESET        CH376S_USB_MODE_RESET
#else
    #define CH37X_USB_MODE_INVALID      CH375_USB_MODE_INVALID
    #define CH37X_USB_MODE_NO_SOF       CH375_USB_MODE_NO_SOF
    #define CH37X_USB_MODE_SOF_AUTO     CH375_USB_MODE_SOF_AUTO
    #define CH37X_USB_MODE_RESET        CH375_USB_MODE_RESET
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - USB Interrupt States
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_USB_INT_SUCCESS       CH376S_USB_INT_SUCCESS
    #define CH37X_USB_INT_CONNECT       CH376S_USB_INT_CONNECT
    #define CH37X_USB_INT_DISCONNECT    CH376S_USB_INT_DISCONNECT
    #define CH37X_USB_INT_BUF_OVER      CH376S_USB_INT_BUF_OVER
    #define CH37X_USB_INT_USB_READY     CH376S_USB_INT_USB_READY
#else
    #define CH37X_USB_INT_SUCCESS       CH375_USB_INT_SUCCESS
    #define CH37X_USB_INT_CONNECT       CH375_USB_INT_CONNECT
    #define CH37X_USB_INT_DISCONNECT    CH375_USB_INT_DISCONNECT
    #define CH37X_USB_INT_BUF_OVER      CH375_USB_INT_BUF_OVER
    #define CH37X_USB_INT_USB_READY     CH375_USB_INT_USB_READY
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - Retry Modes
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_RETRY_TIMES_ZERO      CH376S_RETRY_TIMES_ZERO
    #define CH37X_RETRY_TIMES_2MS       CH376S_RETRY_TIMES_2MS
    #define CH37X_RETRY_TIMES_INFINITY  CH376S_RETRY_TIMES_INFINITY
#else
    #define CH37X_RETRY_TIMES_ZERO      CH375_RETRY_TIMES_ZERO
    #define CH37X_RETRY_TIMES_2MS       CH375_RETRY_TIMES_2MS
    #define CH37X_RETRY_TIMES_INFINITY  CH375_RETRY_TIMES_INFINITY
#endif

/* ==========================================================================
 * UNIFIED MACROS - Command/Data/PID
 * ========================================================================== */
#ifdef USE_CH376S
    #define CH37X_CMD(x)                CH376S_CMD(x)
    #define CH37X_DATA(x)               CH376S_DATA(x)
    #define CH37X_PID2STATUS(x)         CH376S_PID2STATUS(x)
#else
    #define CH37X_CMD(x)                CH375_CMD(x)
    #define CH37X_DATA(x)               CH375_DATA(x)
    #define CH37X_PID2STATUS(x)         CH375_PID2STATUS(x)
#endif

/* ==========================================================================
 * UNIFIED CONSTANTS - USB Speed
 * ========================================================================== */
#ifndef USB_SPEED_SPEED_LS
#define USB_SPEED_SPEED_LS          0x01
#endif
#ifndef USB_SPEED_SPEED_FS
#define USB_SPEED_SPEED_FS          0x00
#endif
#ifndef USB_SPEED_UNKNOWN
#define USB_SPEED_UNKNOWN           0xFF
#endif

/* ==========================================================================
 * UNIFIED API WRAPPERS - Hardware Initialization
 * ========================================================================== */

/**
 * @brief Initialize hardware layer
 */
static inline int ch37x_hwInitManual(const char *name, int uart_index, 
                                      const struct gpio_dt_spec *int_gpio,
                                      uint32_t initial_baudrate,
                                      ch37x_Context_t **ppCtxOut) {
#ifdef USE_CH376S
    return ch376s_hwInitManual(name, uart_index, int_gpio, initial_baudrate,
                               (struct ch376s_Context_t **)ppCtxOut);
#else
    return ch375_hwInitManual(name, uart_index, int_gpio, initial_baudrate,
                              (struct ch375_Context_t **)ppCtxOut);
#endif
}

/**
 * @brief Set hardware baudrate
 */
static inline int ch37x_hwSetBaudrate(ch37x_Context_t *pCtx, uint32_t baudrate) {
#ifdef USE_CH376S
    return ch376s_hwSetBaudrate((struct ch376s_Context_t *)pCtx, baudrate);
#else
    return ch375_hwSetBaudrate((struct ch375_Context_t *)pCtx, baudrate);
#endif
}

/* ==========================================================================
 * UNIFIED API WRAPPERS - Core Functions
 * ========================================================================== */

/**
 * @brief Check if chip exists
 */
static inline int ch37x_checkExist(ch37x_Context_t *pCtx) {
#ifdef USE_CH376S
    return ch376s_checkExist((struct ch376s_Context_t *)pCtx);
#else
    return ch375_checkExist((struct ch375_Context_t *)pCtx);
#endif
}

/**
 * @brief Set USB mode
 */
static inline int ch37x_setUSBMode(ch37x_Context_t *pCtx, uint8_t mode) {
#ifdef USE_CH376S
    return ch376s_setUSBMode((struct ch376s_Context_t *)pCtx, mode);
#else
    return ch375_setUSBMode((struct ch375_Context_t *)pCtx, mode);
#endif
}

/**
 * @brief Set baudrate
 */
static inline int ch37x_setBaudrate(ch37x_Context_t *pCtx, uint32_t baudrate) {
#ifdef USE_CH376S
    return ch376s_setBaudrate((struct ch376s_Context_t *)pCtx, baudrate);
#else
    return ch375_setBaudrate((struct ch375_Context_t *)pCtx, baudrate);
#endif
}

/* ==========================================================================
 * UNIFIED API WRAPPERS - USB Host Commands
 * ========================================================================== */

/**
 * @brief Test device connection
 */
static inline int ch37x_testConnect(ch37x_Context_t *pCtx, uint8_t *pConnStatus) {
#ifdef USE_CH376S
    return ch376s_testConnect((struct ch376s_Context_t *)pCtx, pConnStatus);
#else
    return ch375_testConnect((struct ch375_Context_t *)pCtx, pConnStatus);
#endif
}

/**
 * @brief Get device speed
 */
static inline int ch37x_getDevSpeed(ch37x_Context_t *pCtx, uint8_t *pSpeed) {
#ifdef USE_CH376S
    return ch376s_getDevSpeed((struct ch376s_Context_t *)pCtx, pSpeed);
#else
    return ch375_getDevSpeed((struct ch375_Context_t *)pCtx, pSpeed);
#endif
}

/**
 * @brief Set device speed
 */
static inline int ch37x_setDevSpeed(ch37x_Context_t *pCtx, uint8_t speed) {
#ifdef USE_CH376S
    return ch376s_setDevSpeed((struct ch376s_Context_t *)pCtx, speed);
#else
    return ch375_setDevSpeed((struct ch375_Context_t *)pCtx, speed);
#endif
}

/**
 * @brief Set USB address
 */
static inline int ch37x_setUSBAddr(ch37x_Context_t *pCtx, uint8_t addr) {
#ifdef USE_CH376S
    return ch376s_setUSBAddr((struct ch376s_Context_t *)pCtx, addr);
#else
    return ch375_setUSBAddr((struct ch375_Context_t *)pCtx, addr);
#endif
}

/**
 * @brief Set retry parameters
 */
static inline int ch37x_setRetry(ch37x_Context_t *pCtx, uint8_t times) {
#ifdef USE_CH376S
    return ch376s_setRetry((struct ch376s_Context_t *)pCtx, times);
#else
    return ch375_setRetry((struct ch375_Context_t *)pCtx, times);
#endif
}

/**
 * @brief Send USB token
 */
static inline int ch37x_sendToken(ch37x_Context_t *pCtx, uint8_t ep, bool tog,
                                   uint8_t pid, uint8_t *pStatus) {
#ifdef USE_CH376S
    return ch376s_sendToken((struct ch376s_Context_t *)pCtx, ep, tog, pid, pStatus);
#else
    return ch375_sendToken((struct ch375_Context_t *)pCtx, ep, tog, pid, pStatus);
#endif
}

/**
 * @brief Get interrupt status
 */
static inline int ch37x_getStatus(ch37x_Context_t *pCtx, uint8_t *pStatus) {
#ifdef USE_CH376S
    return ch376s_getStatus((struct ch376s_Context_t *)pCtx, pStatus);
#else
    return ch375_getStatus((struct ch375_Context_t *)pCtx, pStatus);
#endif
}

/**
 * @brief Wait for interrupt
 */
static inline int ch37x_waitInt(ch37x_Context_t *pCtx, uint32_t timeout_ms) {
#ifdef USE_CH376S
    return ch376s_waitInt((struct ch376s_Context_t *)pCtx, timeout_ms);
#else
    return ch375_waitInt((struct ch375_Context_t *)pCtx, timeout_ms);
#endif
}

/* ==========================================================================
 * UNIFIED API WRAPPERS - Data Transfer
 * ========================================================================== */

/**
 * @brief Write block data
 */
static inline int ch37x_writeBlockData(ch37x_Context_t *pCtx, uint8_t *pBuff, uint8_t len) {
#ifdef USE_CH376S
    return ch376s_writeBlockData((struct ch376s_Context_t *)pCtx, pBuff, len);
#else
    return ch375_writeBlockData((struct ch375_Context_t *)pCtx, pBuff, len);
#endif
}

/**
 * @brief Read block data
 */
static inline int ch37x_readBlockData(ch37x_Context_t *pCtx, uint8_t *pBuff, 
                                       uint8_t len, uint8_t *pActualLen) {
#ifdef USE_CH376S
    return ch376s_readBlockData((struct ch376s_Context_t *)pCtx, pBuff, len, pActualLen);
#else
    return ch375_readBlockData((struct ch375_Context_t *)pCtx, pBuff, len, pActualLen);
#endif
}

/**
 * @brief Get private data from context
 */
static inline void *ch37x_getPriv(ch37x_Context_t *pCtx) {
#ifdef USE_CH376S
    return ch376s_getPriv((struct ch376s_Context_t *)pCtx);
#else
    return ch375_getPriv((struct ch375_Context_t *)pCtx);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* CH37X_COMMON_H */