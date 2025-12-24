/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file           ch37x_common.h
 * @brief          Unified interface for CH375 and CH376S
 * @details        Provides compile-time abstraction between the two chips
 */

#ifndef CH37X_COMMON_H
#define CH37X_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Include appropriate chip header */
#ifdef USE_CH376S
    #include "ch376s.h"
    #include "ch376s_uart.h"
#else
    #include "ch375.h"
    #include "ch375_uart.h"
#endif

#include "ch375_host.h"

/* ============================================================================
 * TYPE ALIASES - Map chip-specific types to generic names
 * ============================================================================ */

#ifdef USE_CH376S
    /* CH376S type mappings */
    typedef struct ch376s_Context_t CH37X_Context_t;
    
    #define CH37X_DEFAULT_BAUDRATE      CH376S_DEFAULT_BAUDRATE
    #define CH37X_WORK_BAUDRATE         CH376S_WORK_BAUDRATE
    
    #define CH37X_SUCCESS               CH376S_SUCCESS
    #define CH37X_ERROR                 CH376S_ERROR
    #define CH37X_PARAM_INVALID         CH376S_PARAM_INVALID
    #define CH37X_TIMEOUT               CH376S_TIMEOUT
    
    #define CH37X_USB_MODE_SOF_AUTO     CH376S_USB_MODE_SOF_AUTO
    #define CH37X_USB_INT_SUCCESS       CH376S_USB_INT_SUCCESS
    
    #define CH37X_A_USART_INDEX         CH376S_A_USART_INDEX
    #define CH37X_B_USART_INDEX         CH376S_B_USART_INDEX
    
#else
    /* CH375 type mappings */
    typedef struct ch375_Context_t CH37X_Context_t;
    
    #define CH37X_DEFAULT_BAUDRATE      CH375_DEFAULT_BAUDRATE
    #define CH37X_WORK_BAUDRATE         CH375_WORK_BAUDRATE
    
    #define CH37X_SUCCESS               CH37X_SUCCESS
    #define CH37X_ERROR                 CH375_ERROR
    #define CH37X_PARAM_INVALID         CH375_PARAM_INVALID
    #define CH37X_TIMEOUT               CH375_TIMEOUT
    
    #define CH37X_USB_MODE_SOF_AUTO     CH37X_USB_MODE_SOF_AUTO
    #define CH37X_USB_INT_SUCCESS       CH37X_USB_INT_SUCCESS
    
    #define CH37X_A_USART_INDEX         CH375_A_USART_INDEX
    #define CH37X_B_USART_INDEX         CH375_B_USART_INDEX
#endif

/* ============================================================================
 * FUNCTION WRAPPERS - Unified API for both chips
 * ============================================================================ */

#ifdef USE_CH376S

/* Hardware initialization */
static inline int ch37x_hwInitManual(const char *name, int uart_index,
                                     const struct gpio_dt_spec *int_gpio,
                                     uint32_t initial_baudrate,
                                     CH37X_Context_t **ppCtxOut) {
    return ch376s_hwInitManual(name, uart_index, int_gpio, initial_baudrate, ppCtxOut);
}

static inline int ch37x_hwSetBaudrate(CH37X_Context_t *pCtx, uint32_t baudrate) {
    return ch376s_hwSetBaudrate(pCtx, baudrate);
}

/* Core functions */
static inline int ch37x_checkExist(CH37X_Context_t *pCtx) {
    return ch376s_checkExist(pCtx);
}

static inline int ch37x_setUSBMode(CH37X_Context_t *pCtx, uint8_t mode) {
    return ch376s_setUSBMode(pCtx, mode);
}

static inline int ch37x_setBaudrate(CH37X_Context_t *pCtx, uint32_t baudrate) {
    return ch376s_setBaudrate(pCtx, baudrate);
}

static inline int ch37x_testConnect(CH37X_Context_t *pCtx, uint8_t *pConnStatus) {
    return ch376s_testConnect(pCtx, pConnStatus);
}

static inline int ch37x_getDevSpeed(CH37X_Context_t *pCtx, uint8_t *pSpeed) {
    return ch376s_getDevSpeed(pCtx, pSpeed);
}

static inline int ch37x_setDevSpeed(CH37X_Context_t *pCtx, uint8_t speed) {
    return ch376s_setDevSpeed(pCtx, speed);
}

static inline int ch37x_setUSBAddr(CH37X_Context_t *pCtx, uint8_t addr) {
    return ch376s_setUSBAddr(pCtx, addr);
}

static inline int ch37x_setRetry(CH37X_Context_t *pCtx, uint8_t times) {
    return ch376s_setRetry(pCtx, times);
}

static inline int ch37x_sendToken(CH37X_Context_t *pCtx, uint8_t ep, bool tog,
                                   uint8_t pid, uint8_t *pStatus) {
    return ch376s_sendToken(pCtx, ep, tog, pid, pStatus);
}

static inline int ch37x_writeBlockData(CH37X_Context_t *pCtx, uint8_t *pBuff, uint8_t len) {
    return ch376s_writeBlockData(pCtx, pBuff, len);
}

static inline int ch37x_readBlockData(CH37X_Context_t *pCtx, uint8_t *pBuff, 
                                       uint8_t len, uint8_t *pActualLen) {
    return ch376s_readBlockData(pCtx, pBuff, len, pActualLen);
}

#else /* CH375 */

/* Hardware initialization */
static inline int ch37x_hwInitManual(const char *name, int uart_index,
                                     const struct gpio_dt_spec *int_gpio,
                                     uint32_t initial_baudrate,
                                     CH37X_Context_t **ppCtxOut) {
    return ch375_hwInitManual(name, uart_index, int_gpio, initial_baudrate, ppCtxOut);
}

static inline int ch37x_hwSetBaudrate(CH37X_Context_t *pCtx, uint32_t baudrate) {
    return ch375_hwSetBaudrate(pCtx, baudrate);
}

/* Core functions */
static inline int ch37x_checkExist(CH37X_Context_t *pCtx) {
    return ch375_checkExist(pCtx);
}

static inline int ch37x_setUSBMode(CH37X_Context_t *pCtx, uint8_t mode) {
    return ch375_setUSBMode(pCtx, mode);
}

static inline int ch37x_setBaudrate(CH37X_Context_t *pCtx, uint32_t baudrate) {
    return ch375_setBaudrate(pCtx, baudrate);
}

#endif

/* ============================================================================
 * HOST LAYER - These functions are shared between both chips
 * ============================================================================ */

/* Host layer uses the same interface regardless of chip type */
#define ch37x_hostInit              ch375_hostInit
#define ch37x_hostWaitDeviceConnect ch375_hostWaitDeviceConnect
#define ch37x_hostUdevOpen          ch375_hostUdevOpen
#define ch37x_hostUdevClose         ch375_hostUdevClose
#define ch37x_hostResetDev          ch375_hostResetDev
#define ch37x_hostControlTransfer   ch375_hostControlTransfer
#define ch37x_hostBulkTransfer      ch375_hostBulkTransfer
#define ch37x_hostClearStall        ch375_hostClearStall
#define ch37x_hostSetConfiguration  ch375_hostSetConfiguration

/* Return codes */
#define CH37X_HOST_SUCCESS          CH375_HOST_SUCCESS
#define CH37X_HOST_ERROR            CH375_HOST_ERROR
#define CH37X_HOST_PARAM_INVALID    CH375_HOST_PARAM_INVALID
#define CH37X_HOST_TIMEOUT          CH375_HOST_TIMEOUT
#define CH37X_HOST_DEV_DISCONNECT   CH375_HOST_DEV_DISCONNECT

#ifdef __cplusplus
}
#endif

#endif /* CH37X_COMMON_H */