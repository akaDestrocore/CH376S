/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                          GhostHIDe Project                            ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 * 
 * @file           ch376s.h
 * @brief          CH376S USB host controller driver core interface
 * 
 * @author         destrocore
 * @date           2025
 * 
 * @details
 * Core driver for the CH376S USB host controller chip. Provides low-level
 * communication primitives for USB device enumeration and data transfer
 * over standard 8-bit UART using command sync bytes (0x57, 0xAB).
 * 
 * @copyright 
 * Copyright (c) 2025 akaDestrocore
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CH376S_H
#define CH376S_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "usb.h"

#define CH376S_WAIT_INT_TIMEOUT_MS 2000
#define CH376S_CHECK_EXIST_DATA1 0x65
#define CH376S_CHECK_EXIST_DATA2 ((uint8_t)~CH376S_CHECK_EXIST_DATA1)

/**
 * @brief CH376S commands
 */
typedef enum {
    CH376S_CMD_GET_IC_VER        =   0x01,
    CH376S_CMD_SET_BAUDRATE      =   0x02,
    CH376S_CMD_SET_USB_SPEED     =   0x04,
    CH376S_CMD_CHECK_EXIST       =   0x06,
    CH376S_CMD_GET_DEV_RATE      =   0x0A,
    CH376S_CMD_SET_RETRY         =   0x0B,
    CH376S_CMD_SET_USB_ADDR      =   0x13,
    CH376S_CMD_SET_USB_MODE      =   0x15,
    CH376S_CMD_TEST_CONNECT      =   0x16,
    CH376S_CMD_ABORT_NAK         =   0x17,
    CH376S_CMD_SET_ENDP6         =   0x1C,
    CH376S_CMD_SET_ENDP7         =   0x1D,
    CH376S_CMD_GET_STATUS        =   0x22,
    CH376S_CMD_UNLOCK_USB        =   0x23,
    CH376S_CMD_RD_USB_DATA0      =   0x27,
    CH376S_CMD_RD_USB_DATA       =   0x28,
    CH376S_CMD_WR_USB_DATA7      =   0x2B,
    CH376S_CMD_WR_HOST_DATA      =   0x2C,
    CH376S_CMD_GET_DESC          =   0x46,
    CH376S_CMD_ISSUE_TKN_X       =   0x4E,
    CH376S_CMD_ISSUE_TOKEN       =   0x4F,
    CH376S_CMD_RET_SUCCESS       =   0x51,
    CH376S_CMD_RET_FAILED        =   0x5F
} ch376s_CMD_e;

/**
 * @brief CH376S USB host modes
 */
typedef enum {
    CH376S_USB_MODE_INVALID      =   0x04,
    CH376S_USB_MODE_NO_SOF       =   0x05,
    CH376S_USB_MODE_SOF_AUTO     =   0x06,
    CH376S_USB_MODE_RESET        =   0x07
} ch376s_USBHostMode_e;

/**
 * @brief CH376S USB host interrupt states
 */
typedef enum {
    CH376S_USB_INT_SUCCESS       =   0x14,
    CH376S_USB_INT_CONNECT       =   0x15,
    CH376S_USB_INT_DISCONNECT    =   0x16,
    CH376S_USB_INT_BUF_OVER      =   0x17,
    CH376S_USB_INT_USB_READY     =   0x18
} ch376s_USBHostInt_e;

#define CH376S_PID2STATUS(x) ((x) | 0x20)

/**
 * @brief CH376S error codes
 */
typedef enum {
    CH376S_SUCCESS               =   0,
    CH376S_ERROR                 =   -1,
    CH376S_PARAM_INVALID         =   -2,
    CH376S_WRITE_CMD_FAILED      =   -3,
    CH376S_READ_DATA_FAILED      =   -4,
    CH376S_NO_EXIST              =   -5,
    CH376S_TIMEOUT               =   -6,
    CH376S_NOT_FOUND             =   -7,
} ch376s_ErrNo;

/**
 * @brief CH376S retry
 */
typedef enum {
    CH376S_RETRY_TIMES_ZERO      =   0x00,
    CH376S_RETRY_TIMES_2MS       =   0x01,
    CH376S_RETRY_TIMES_INFINITY  =   0x02
} ch376s_Retry_e;

/* Default Baudrates */
#define CH376S_DEFAULT_BAUDRATE  9600
#define CH376S_WORK_BAUDRATE     115200

// Forward declaration
struct ch376s_Context_t;

// Function pointer types
typedef int (*ch376s_writeByteFn_t)(struct ch376s_Context_t *pCtx, uint8_t byte);
typedef int (*ch376s_readByteFn_t)(struct ch376s_Context_t *pCtx, uint8_t *byte);
typedef int (*ch376s_queryIntFn_t)(struct ch376s_Context_t *pCtx);

/**
 * @brief CH376S Context structure
 */
struct ch376s_Context_t {
    void *priv;
    ch376s_writeByteFn_t write_byte;
    ch376s_readByteFn_t read_byte;
    ch376s_queryIntFn_t query_int;
    struct k_mutex lock;
};

/**
 * @brief CH376S core functions
 */
int ch376s_openContext(struct ch376s_Context_t **ppCtx,
                       ch376s_writeByteFn_t write_byte,
                       ch376s_readByteFn_t read_byte,
                       ch376s_queryIntFn_t query_int,
                       void *priv);
int ch376s_closeContext(struct ch376s_Context_t *pCtx);
void *ch376s_getPriv(struct ch376s_Context_t *pCtx);

/**
 * @brief Transfer commands
 */
int ch376s_checkExist(struct ch376s_Context_t *pCtx);
int ch376s_getVersion(struct ch376s_Context_t *pCtx, uint8_t *pVersion);
int ch376s_setBaudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate);
int ch376s_setUSBMode(struct ch376s_Context_t *pCtx, uint8_t mode);
int ch376s_getStatus(struct ch376s_Context_t *pCtx, uint8_t *pStatus);
int ch376s_abortNAK(struct ch376s_Context_t *pCtx);
int ch376s_queryInt(struct ch376s_Context_t *pCtx);
int ch376s_waitInt(struct ch376s_Context_t *pCtx, uint32_t timeout_ms);

/**
 * @brief Host commands
 */
int ch376s_testConnect(struct ch376s_Context_t *pCtx, uint8_t *pConnStatus);
int ch376s_getDevSpeed(struct ch376s_Context_t *pCtx, uint8_t *pSpeed);
int ch376s_setDevSpeed(struct ch376s_Context_t *pCtx, uint8_t speed);
int ch376s_setUSBAddr(struct ch376s_Context_t *pCtx, uint8_t addr);
int ch376s_setRetry(struct ch376s_Context_t *pCtx, uint8_t times);
int ch376s_sendToken(struct ch376s_Context_t *pCtx, uint8_t ep, bool tog,
                    uint8_t pid, uint8_t *pStatus);

/**
 * @brief Data transfer commands
 */
int ch376s_writeByte(struct ch376s_Context_t *pCtx, uint8_t byte);
int ch376s_readByte(struct ch376s_Context_t *pCtx, uint8_t *pByte);
int ch376s_writeBlockData(struct ch376s_Context_t *pCtx, uint8_t *pBuff, uint8_t len);
int ch376s_readBlockData(struct ch376s_Context_t *pCtx, uint8_t *pBuff, 
                         uint8_t len, uint8_t *pActualLen);

#ifdef __cplusplus
}
#endif

#endif /* CH376S_H */