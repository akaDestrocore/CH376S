/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                          GhostHIDe Project                            ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 * 
 * @file           ch376s.c
 * @brief          CH376S USB host controller driver core implementation
 * 
 * @author         destrocore
 * @date           2025
 * 
 * @details
 * Implements the CH376S protocol using standard 8-bit UART with Zephyr's
 * UART API. Commands are prefixed with sync bytes (0x57, 0xAB) for
 * command/data differentiation.
 * 
 * @copyright 
 * Copyright (c) 2025 akaDestrocore
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ch376s.h"

LOG_MODULE_REGISTER(ch376s, LOG_LEVEL_DBG);

/* --------------------------------------------------------------------------
 * CH376S core functions
 * -------------------------------------------------------------------------*/
 
int ch376s_openContext(struct ch376s_Context_t **ppCtx,
                       ch376s_writeByteFn_t write_byte,
                       ch376s_readByteFn_t read_byte,
                       ch376s_queryIntFn_t query_int,
                       void *priv) {
    struct ch376s_Context_t *new_ctx;

    if (NULL == ppCtx || NULL == write_byte || NULL == read_byte || NULL == query_int) {
        LOG_ERR("Invalid parameters!");
        return CH376S_PARAM_INVALID;           
    }

    new_ctx = k_malloc(sizeof(struct ch376s_Context_t));
    if (NULL == new_ctx) {
        LOG_ERR("Failed to allocate memory for context!");
        return CH376S_ERROR;
    }

    memset(new_ctx, 0x00, sizeof(struct ch376s_Context_t));
    k_mutex_init(&new_ctx->lock);

    new_ctx->priv = priv;
    new_ctx->write_byte = write_byte;
    new_ctx->read_byte = read_byte;
    new_ctx->query_int = query_int;

    *ppCtx = new_ctx;
    return CH376S_SUCCESS;
}

int ch376s_closeContext(struct ch376s_Context_t *pCtx) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    k_free(pCtx);
    return CH376S_SUCCESS;
}

void *ch376s_getPriv(struct ch376s_Context_t *pCtx) {
    if (NULL == pCtx) {
        return NULL;
    }
    return pCtx->priv;
}

/* --------------------------------------------------------------------------
 * Transfer commands
 * -------------------------------------------------------------------------*/

int ch376s_checkExist(struct ch376s_Context_t *pCtx) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    uint8_t recvBuff = 0;
    int ret = -1;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_CHECK_EXIST);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, CH376S_CHECK_EXIST_DATA1);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_readByte(pCtx, &recvBuff);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }

    k_mutex_unlock(&pCtx->lock);

    if (CH376S_CHECK_EXIST_DATA2 != recvBuff) {
        LOG_ERR("Expected 0x%02X, but got 0x%02X!", CH376S_CHECK_EXIST_DATA2, recvBuff);
        return CH376S_NO_EXIST;
    }

    return CH376S_SUCCESS;
}

int ch376s_getVersion(struct ch376s_Context_t *pCtx, uint8_t *pVersion) {
    if (NULL == pCtx || NULL == pVersion) {
        LOG_ERR("Invalid parameters!");
        return CH376S_PARAM_INVALID;
    }

    uint8_t ver = 0;
    int ret = -1;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_GET_IC_VER);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_readByte(pCtx, &ver);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }

    *pVersion = ver & 0x3F;
    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_setBaudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t data1 = 0;
    uint8_t data2 = 0;

    switch(baudrate) {
        case 9600: {
            data1 = 0x02;
            data2 = 0xB2;
            break;
        }
        case 115200: {
            data1 = 0x03;
            data2 = 0xCC;
            break;
        }
        default: {
            LOG_ERR("Unsupported baudrate: %" PRIu32 "", baudrate);
            return CH376S_PARAM_INVALID;
        }
    }

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_SET_BAUDRATE);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, data1);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, data2);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_setUSBMode(struct ch376s_Context_t *pCtx, uint8_t mode) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t usb_mode = 0;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_SET_USB_MODE);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, mode);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_readByte(pCtx, &usb_mode);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }

    k_mutex_unlock(&pCtx->lock);

    if (CH376S_CMD_RET_SUCCESS != usb_mode) {
        LOG_ERR("Set USB mode failed: ret=0x%02X", usb_mode);
        return CH376S_ERROR;
    }

    return CH376S_SUCCESS;
}

int ch376s_getStatus(struct ch376s_Context_t *pCtx, uint8_t *pStatus) {
    if (NULL == pCtx || NULL == pStatus) {
        LOG_ERR("Invalid parameters!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t status = -1;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_GET_STATUS);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_readByte(pCtx, &status);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }

    *pStatus = status;
    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_abortNAK(struct ch376s_Context_t *pCtx) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;

    k_mutex_lock(&pCtx->lock, K_FOREVER);
    ret = ch376s_writeByte(pCtx, CH376S_CMD_ABORT_NAK);
    k_mutex_unlock(&pCtx->lock);

    return ret == CH376S_SUCCESS ? CH376S_SUCCESS : CH376S_WRITE_CMD_FAILED;
}

int ch376s_queryInt(struct ch376s_Context_t *pCtx) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return 0;
    }
    return pCtx->query_int(pCtx);
}

int ch376s_waitInt(struct ch376s_Context_t *pCtx, uint32_t timeout_ms) {
    int ret = -1;
    uint32_t start = k_uptime_get_32();
    uint32_t pollCount = 0;
    uint8_t status = 0xFF;
    uint8_t lastStatus = 0xFF;
    
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }
    
    ret = ch376s_getStatus(pCtx, &status);
    if (CH376S_SUCCESS == ret) {
        lastStatus = status;
        
        if (status == CH376S_USB_INT_SUCCESS || status == CH376S_USB_INT_CONNECT ||
            status == CH376S_USB_INT_DISCONNECT || status == CH376S_USB_INT_USB_READY ||
            status == CH376S_PID2STATUS(USB_PID_NAK) || status == CH376S_PID2STATUS(USB_PID_STALL) ||
            status == CH376S_PID2STATUS(USB_PID_ACK)) {
            return CH376S_SUCCESS;
        }
    }
    
    while ((k_uptime_get_32() - start) < timeout_ms) {
        pollCount++;
        ret = ch376s_getStatus(pCtx, &status);
        
        if (CH376S_SUCCESS == ret) {
            if (status != lastStatus) {
                lastStatus = status;
            }
            
            if (status == CH376S_USB_INT_SUCCESS || status == CH376S_USB_INT_CONNECT ||
                status == CH376S_USB_INT_DISCONNECT || status == CH376S_USB_INT_USB_READY ||
                status == CH376S_PID2STATUS(USB_PID_NAK) || status == CH376S_PID2STATUS(USB_PID_STALL) ||
                status == CH376S_PID2STATUS(USB_PID_ACK)) {
                return CH376S_SUCCESS;
            }
        }
        
        if (pollCount < 100) {
            k_busy_wait(500);
        } else if (pollCount < 1000) {
            k_busy_wait(1000);
        } else {
            k_msleep(2);
        }
    }

    ret = ch376s_getStatus(pCtx, &status);
    LOG_ERR("Polling timeout after %u ms (%u polls, final_status=0x%02X, ret=%d)", 
            timeout_ms, pollCount, status, ret);
    
    return CH376S_TIMEOUT;
}

/* --------------------------------------------------------------------------
 * Host commands
 * -------------------------------------------------------------------------*/

int ch376s_testConnect(struct ch376s_Context_t *pCtx, uint8_t *pConnStatus) {
    if (NULL == pCtx || NULL == pConnStatus) {
        LOG_ERR("Invalid parameters!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t status = 0;
    uint8_t buff = 0;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_TEST_CONNECT);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    k_msleep(1);

    ret = ch376s_readByte(pCtx, &buff);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }
    
    if (CH376S_USB_INT_DISCONNECT != buff && CH376S_USB_INT_CONNECT != buff &&
        CH376S_USB_INT_USB_READY != buff) {
        buff = CH376S_USB_INT_DISCONNECT;
    }

    if (CH376S_USB_INT_DISCONNECT == buff) {
        ch376s_getStatus(pCtx, &status);
    }

    k_mutex_unlock(&pCtx->lock);
    *pConnStatus = buff;
    return CH376S_SUCCESS;
}

int ch376s_getDevSpeed(struct ch376s_Context_t *pCtx, uint8_t *pSpeed) {
    if (NULL == pCtx || NULL == pSpeed) {
        LOG_ERR("Invalid parameters!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t devSpeed;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_GET_DEV_RATE);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, 0x07);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_readByte(pCtx, &devSpeed);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }

    *pSpeed = (devSpeed & 0x10) ? USB_SPEED_SPEED_LS : USB_SPEED_SPEED_FS;

    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_setDevSpeed(struct ch376s_Context_t *pCtx, uint8_t speed) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t devSpeed = 0;

    if (USB_SPEED_SPEED_LS != speed && USB_SPEED_SPEED_FS != speed) {
        LOG_ERR("Invalid speed value: 0x%02X", speed);
        return CH376S_PARAM_INVALID;
    }

    devSpeed = (speed == USB_SPEED_SPEED_LS ? 0x02 : 0x00);

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_SET_USB_SPEED);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, devSpeed);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_setUSBAddr(struct ch376s_Context_t *pCtx, uint8_t addr) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    
    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_SET_USB_ADDR);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, addr);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_setRetry(struct ch376s_Context_t *pCtx, uint8_t times) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t param;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_SET_RETRY);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, 0x25);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    if (0 == times) {
        param = 0x05;
    } else if (1 == times) {
        param = 0xC0;
    } else {
        param = 0x85;
    }

    ret = ch376s_writeByte(pCtx, param);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }
    
    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_sendToken(struct ch376s_Context_t *pCtx, uint8_t ep, bool tog,
                     uint8_t pid, uint8_t *pStatus) {
    if (NULL == pCtx || NULL == pStatus) {
        LOG_ERR("Invalid parameters");
        return CH376S_PARAM_INVALID;
    }

    int ret = -1;
    uint8_t togVal = tog ? 0xC0 : 0x00;
    uint8_t epPID = (ep << 4) | pid;
    uint8_t status = -1;

    k_mutex_lock(&pCtx->lock, K_FOREVER);

    ret = ch376s_writeByte(pCtx, CH376S_CMD_ISSUE_TKN_X);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, togVal);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, epPID);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    k_mutex_unlock(&pCtx->lock);

    if (USB_PID_IN != pid) {
        k_busy_wait(500);
    }

    ret = ch376s_waitInt(pCtx, CH376S_WAIT_INT_TIMEOUT_MS);
    if (CH376S_SUCCESS != ret) {
        return CH376S_TIMEOUT;
    }

    ret = ch376s_getStatus(pCtx, &status);
    if (CH376S_SUCCESS != ret) {
        return CH376S_ERROR;
    }

    *pStatus = status;
    return CH376S_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Data transfer commands
 * -------------------------------------------------------------------------*/

int ch376s_writeByte(struct ch376s_Context_t *pCtx, uint8_t byte) {
    if (NULL == pCtx) {
        LOG_ERR("Invalid context!");
        return CH376S_PARAM_INVALID;
    }
    return pCtx->write_byte(pCtx, byte);
}

int ch376s_readByte(struct ch376s_Context_t *pCtx, uint8_t *pByte) {
    if (NULL == pCtx || NULL == pByte) {
        LOG_ERR("Invalid parameters!");
        return CH376S_PARAM_INVALID;
    }
    return pCtx->read_byte(pCtx, pByte);
}

int ch376s_writeBlockData(struct ch376s_Context_t *pCtx, uint8_t *pBuff, uint8_t len) {
    int ret = -1;
    uint8_t offset;

    if (NULL == pCtx) {
        return CH376S_PARAM_INVALID;
    }

    if (NULL == pBuff && 0 != len) {
        return CH376S_PARAM_INVALID;
    }

    k_mutex_lock(&pCtx->lock, K_FOREVER);
    
    ret = ch376s_writeByte(pCtx, CH376S_CMD_WR_HOST_DATA);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    ret = ch376s_writeByte(pCtx, len);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }

    offset = 0;
    while (len > 0) {
        ret = ch376s_writeByte(pCtx, pBuff[offset]);
        if (CH376S_SUCCESS != ret) {
            k_mutex_unlock(&pCtx->lock);
            return CH376S_WRITE_CMD_FAILED;
        }
        offset++;
        len--;
    }

    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}

int ch376s_readBlockData(struct ch376s_Context_t *pCtx, uint8_t *pBuff, 
                         uint8_t len, uint8_t *pActualLen) {
    int ret = -1;
    uint8_t dataLen;
    uint8_t resiLen;
    uint8_t offset;
    
    if (NULL == pCtx || NULL == pBuff || NULL == pActualLen) {
        LOG_ERR("Invalid parameters");
        return CH376S_PARAM_INVALID;
    }
    
    k_mutex_lock(&pCtx->lock, K_FOREVER);
    
    ret = ch376s_writeByte(pCtx, CH376S_CMD_RD_USB_DATA);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_WRITE_CMD_FAILED;
    }
    
    ret = ch376s_readByte(pCtx, &dataLen);
    if (CH376S_SUCCESS != ret) {
        k_mutex_unlock(&pCtx->lock);
        return CH376S_READ_DATA_FAILED;
    }
    
    resiLen = dataLen;
    offset = 0;
    
    while (resiLen > 0 && offset < len) {
        ret = ch376s_readByte(pCtx, &pBuff[offset]);
        
        if (CH376S_TIMEOUT == ret) {
            break;
        }
        
        if (CH376S_SUCCESS != ret) {
            LOG_ERR("Read failed at offset %d: %d", offset, ret);
            k_mutex_unlock(&pCtx->lock);
            return CH376S_READ_DATA_FAILED;
        }
        
        offset++;
        resiLen--;
    }
    
    *pActualLen = offset;
    
    k_mutex_unlock(&pCtx->lock);
    return CH376S_SUCCESS;
}