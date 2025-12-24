/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                          GhostHIDe Project                            ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 * 
 * @file           ch376s_uart_rp2.c
 * @brief          CH376S UART hardware interface (RP2 PIO 8-bit)
 * 
 * @author         destrocore
 * @date           2025
 * 
 * @details
 * PIO-based 8-bit UART implementation for CH376S on RP2040/RP2350. Uses
 * standard 8-bit UART without 9th bit, simplified from CH375 implementation.
 * 
 * @copyright 
 * Copyright (c) 2025 akaDestrocore
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ch376s_uart.h"

LOG_MODULE_DECLARE(ch376s_uart);

/* --------------------------------------------------------------------------
 * PIO Programs for Standard 8-bit UART
 * -------------------------------------------------------------------------*/

// Standard 8-bit UART TX program
#define uart_tx_8bit_wrap_target 0
#define uart_tx_8bit_wrap 5
#define uart_tx_8bit_pio_version 0

static const uint16_t uart_tx_8bit_program_instructions[] = {
            //     .wrap_target
    0x80a0, //  0: pull   block
    0xe027, //  1: set    x, 7
    0xe700, //  2: set    pins, 0                [7]
    0x6001, //  3: out    pins, 1
    0x0643, //  4: jmp    x--, 3                 [6]
    0xe701, //  5: set    pins, 1                [7]
            //     .wrap
};

static const struct pio_program uart_tx_8bit_program = {
    .instructions = uart_tx_8bit_program_instructions,
    .length = 6,
    .origin = -1,
    .pio_version = uart_tx_8bit_pio_version,
};

static inline pio_sm_config uart_tx_8bit_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + uart_tx_8bit_wrap_target, offset + uart_tx_8bit_wrap);
    return c;
}

static inline void uart_tx_8bit_program_init(PIO pio, uint sm, uint offset, uint pin_tx, uint baud) {
    pio_sm_set_consecutive_pindirs(pio, sm, pin_tx, 1, true);
    pio_gpio_init(pio, pin_tx);
    
    pio_sm_config c = uart_tx_8bit_program_get_default_config(offset);
    sm_config_set_out_pins(&c, pin_tx, 1);
    sm_config_set_set_pins(&c, pin_tx, 1);
    
    // Shift right, threshold 8
    sm_config_set_out_shift(&c, true, false, 8);
    
    // 8 cycles per bit
    float div = (float)clock_get_hz(clk_sys) / (8.0f * (float)baud);
    sm_config_set_clkdiv(&c, div);
    
    // FIFO join for TX
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Standard 8-bit UART RX program
#define uart_rx_8bit_wrap_target 0
#define uart_rx_8bit_wrap 4
#define uart_rx_8bit_pio_version 0

static const uint16_t uart_rx_8bit_program_instructions[] = {
            //     .wrap_target
    0x2020, //  0: wait   0 pin, 0
    0xeb27, //  1: set    x, 7                   [11]
    0x4001, //  2: in     pins, 1
    0x0642, //  3: jmp    x--, 2                 [6]
    0x20a0, //  4: wait   1 pin, 0
            //     .wrap
};

static const struct pio_program uart_rx_8bit_program = {
    .instructions = uart_rx_8bit_program_instructions,
    .length = 5,
    .origin = -1,
    .pio_version = uart_rx_8bit_pio_version,
};

static inline pio_sm_config uart_rx_8bit_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + uart_rx_8bit_wrap_target, offset + uart_rx_8bit_wrap);
    return c;
}

static inline void uart_rx_8bit_program_init(PIO pio, uint sm, uint offset, uint pin_rx, uint baud) {
    pio_sm_set_consecutive_pindirs(pio, sm, pin_rx, 1, false);
    pio_gpio_init(pio, pin_rx);
    gpio_pull_up(pin_rx);
    
    pio_sm_config c = uart_rx_8bit_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin_rx);
    sm_config_set_jmp_pin(&c, pin_rx);
    
    // Shift right, autopush at 8 bits
    sm_config_set_in_shift(&c, true, true, 8);
    
    // 8 cycles per bit
    float div = (float)clock_get_hz(clk_sys) / (8.0f * (float)baud);
    sm_config_set_clkdiv(&c, div);
    
    // FIFO join for RX
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

/* Private function prototypes -----------------------------------------------*/
static int load_pio_programs(ch376s_HwContext_t *hw);
static int init_gpio_sequence(ch376s_HwContext_t *hw);
static int configure_state_machines(ch376s_HwContext_t *hw, uint32_t baudrate);
static void flush_startup_transients(ch376s_HwContext_t *hw);

static int ch376s_write_data_cb(struct ch376s_Context_t *pCtx, uint8_t data);
static int ch376s_read_data_cb(struct ch376s_Context_t *pCtx, uint8_t *pData);
static int ch376s_query_int_cb(struct ch376s_Context_t *pCtx);

static int pio_write_8bit(ch376s_HwContext_t *hw, uint8_t data);
static int pio_read_8bit(ch376s_HwContext_t *hw, uint8_t *data, k_timeout_t timeout);

/**
 * @brief Initialize CH376S hardware on RP2
 */
int ch376s_rp2_hw_init(const char *name, int uart_idx,
                        const struct gpio_dt_spec *int_gpio,
                        uint32_t baudrate,
                        struct ch376s_Context_t **ppCtxOut) {
    int ret = -1;
    ch376s_HwContext_t *hw = NULL;
    struct ch376s_Context_t *pCtx = NULL;

    if (CH376S_A_USART_INDEX != uart_idx && CH376S_B_USART_INDEX != uart_idx) {
        LOG_ERR("Invalid UART index: %d (must be 0 or 1)", uart_idx);
        return -EINVAL;
    }

    // Allocate context
    hw = k_malloc(sizeof(ch376s_HwContext_t));
    if (NULL == hw) {
        LOG_ERR("Failed to allocate hardware context");
        return -ENOMEM;
    }
    memset(hw, 0x00, sizeof(ch376s_HwContext_t));

    hw->name = name;
    hw->baudrate = baudrate;

    // CH376S_A
    if (CH376S_A_USART_INDEX == uart_idx) {
        hw->pio = pio0;
        hw->tx_pin = PIO_UART_TX_PIN_CH376S_A;
        hw->rx_pin = PIO_UART_RX_PIN_CH376S_A;
        hw->sm_tx = PIO_UART_SM_TX;
        hw->sm_rx = PIO_UART_SM_RX;
    } else {
        // CH376S_B
        hw->pio = pio1;
        hw->tx_pin = PIO_UART_TX_PIN_CH376S_B;
        hw->rx_pin = PIO_UART_RX_PIN_CH376S_B;
        hw->sm_tx = PIO_UART_SM_TX;
        hw->sm_rx = PIO_UART_SM_RX;
    }

    // Store INT pin
    if (NULL != int_gpio) {
        hw->int_gpio = *int_gpio;
    } else {
        memset(&hw->int_gpio, 0x00, sizeof(hw->int_gpio));
    }

    // Load PIO programs
    ret = load_pio_programs(hw);
    if (ret < 0) {
        LOG_ERR("%s: Failed to load PIO programs: %d", name, ret);
        k_free(hw);
        return ret;
    }

    // Initialize GPIO
    ret = init_gpio_sequence(hw);
    if (ret < 0) {
        LOG_ERR("%s: GPIO init failed: %d", name, ret);
        k_free(hw);
        return ret;
    }

    // Configure state machines
    ret = configure_state_machines(hw, baudrate);
    if (ret < 0) {
        LOG_ERR("%s: SM config failed: %d", name, ret);
        k_free(hw);
        return ret;
    }

    // Flush startup transients
    flush_startup_transients(hw);

    // Open CH376S context
    ret = ch376s_openContext(&pCtx, ch376s_write_data_cb, ch376s_read_data_cb,
                              ch376s_query_int_cb, hw);
    if (CH376S_SUCCESS != ret) {
        LOG_ERR("%s: ch376s_openContext failed: %d", name, ret);
        k_free(hw);
        return -EIO;
    }

    *ppCtxOut = pCtx;
    LOG_INF("%s: RP2 PIO 8-bit UART initialized successfully", name);
    return 0;
}

/**
 * @brief Set baudrate
 */
int ch376s_rp2_set_baudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);

    if (NULL == hw) {
        return -EINVAL;
    }

    // Disable SMs
    pio_sm_set_enabled(hw->pio, hw->sm_tx, false);
    pio_sm_set_enabled(hw->pio, hw->sm_rx, false);

    k_msleep(10);

    // Clear FIFOs
    pio_sm_clear_fifos(hw->pio, hw->sm_tx);
    pio_sm_clear_fifos(hw->pio, hw->sm_rx);

    // Reconfigure
    int ret = configure_state_machines(hw, baudrate);
    if (ret < 0) {
        LOG_ERR("%s: Failed to reconfigure for new baudrate", hw->name);
        return ret;
    }

    hw->baudrate = baudrate;

    // Flush again
    flush_startup_transients(hw);
    return 0;
}

/* --------------------------------------------------------------------------
 * Private Helper Functions
 * -------------------------------------------------------------------------*/

static int load_pio_programs(ch376s_HwContext_t *hw) {
    if (true != pio_can_add_program(hw->pio, &uart_tx_8bit_program)) {
        LOG_ERR("%s: No space for TX program", hw->name);
        return -ENOMEM;
    }

    if (true != pio_can_add_program(hw->pio, &uart_rx_8bit_program)) {
        LOG_ERR("%s: No space for RX program", hw->name);
        return -ENOMEM;
    }

    hw->offset_tx = pio_add_program(hw->pio, &uart_tx_8bit_program);
    hw->offset_rx = pio_add_program(hw->pio, &uart_rx_8bit_program);

    pio_sm_claim(hw->pio, hw->sm_tx);
    pio_sm_claim(hw->pio, hw->sm_rx);

    return 0;
}

static int init_gpio_sequence(ch376s_HwContext_t *hw) {
    // TX pin
    gpio_init(hw->tx_pin);
    gpio_set_dir(hw->tx_pin, GPIO_OUT);
    gpio_put(hw->tx_pin, 1);

    // RX pin
    gpio_init(hw->rx_pin);
    gpio_set_dir(hw->rx_pin, GPIO_IN);
    gpio_pull_up(hw->rx_pin);

    k_msleep(5);

    // Transfer to PIO
    pio_gpio_init(hw->pio, hw->tx_pin);
    pio_gpio_init(hw->pio, hw->rx_pin);

    // Set TX as output
    pio_sm_set_consecutive_pindirs(hw->pio, hw->sm_tx, hw->tx_pin, 1, true);

    return 0;
}

static int configure_state_machines(ch376s_HwContext_t *hw, uint32_t baudrate) {
    // Configure RX before TX
    uart_rx_8bit_program_init(hw->pio, hw->sm_rx, hw->offset_rx, hw->rx_pin, baudrate);
    k_busy_wait(100);

    uart_tx_8bit_program_init(hw->pio, hw->sm_tx, hw->offset_tx, hw->tx_pin, baudrate);

    return 0;
}

static void flush_startup_transients(ch376s_HwContext_t *hw) {
    pio_sm_put_blocking(hw->pio, hw->sm_tx, 0xFFu);
    k_msleep(5);

    for (int i = 0; i < 4; i++) {
        pio_sm_clear_fifos(hw->pio, hw->sm_rx);
        k_msleep(5);
    }

    k_msleep(300);
}

static int pio_write_8bit(ch376s_HwContext_t *hw, uint8_t data) {
    int64_t start = k_uptime_get();
    while (pio_sm_is_tx_fifo_full(hw->pio, hw->sm_tx)) {
        if ((k_uptime_get() - start) > 100) {
            LOG_ERR("%s: TX FIFO full timeout", hw->name);
            return -ETIMEDOUT;
        }
        k_busy_wait(10);
    }

    pio_sm_put_blocking(hw->pio, hw->sm_tx, (uint32_t)data);
    k_busy_wait(800);
    return 0;
}

static int pio_read_8bit(ch376s_HwContext_t *hw, uint8_t *data, k_timeout_t timeout) {
    if (NULL == data) {
        return -EINVAL;
    }

    int64_t timeout_ms;
    if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
        timeout_ms = INT64_MAX;
    } else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
        timeout_ms = 0;
    } else {
        timeout_ms = timeout.ticks * 1000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
    }

    int64_t start = k_uptime_get();
    while (pio_sm_is_rx_fifo_empty(hw->pio, hw->sm_rx)) {
        if ((k_uptime_get() - start) >= timeout_ms) {
            return -ETIMEDOUT;
        }
        k_busy_wait(10);
    }

    uint32_t raw = pio_sm_get_blocking(hw->pio, hw->sm_rx);
    *data = (uint8_t)((raw >> 24) & 0xFFu);

    return 0;
}

/* --------------------------------------------------------------------------
 * CH376S Callback Functions
 * -------------------------------------------------------------------------*/

static int ch376s_write_data_cb(struct ch376s_Context_t *pCtx, uint8_t data) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);

    if (!hw) {
        return CH376S_ERROR;
    }

    int ret = pio_write_8bit(hw, data);
    if (ret < 0) {
        LOG_ERR("%s: Write failed: %d", hw->name, ret);
        return CH376S_ERROR;
    }

    return CH376S_SUCCESS;
}

static int ch376s_read_data_cb(struct ch376s_Context_t *pCtx, uint8_t *pData) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);

    if (!hw || !pData) {
        return CH376S_ERROR;
    }

    uint8_t val;
    int ret = pio_read_8bit(hw, &val, K_MSEC(50));

    if (ret < 0) {
        if (ret == -ETIMEDOUT) {
            LOG_DBG("%s: Read timeout", hw->name);
            return CH376S_TIMEOUT;
        }
        LOG_ERR("%s: Read failed: %d", hw->name, ret);
        return CH376S_ERROR;
    }

    *pData = val;
    return CH376S_SUCCESS;
}

static int ch376s_query_int_cb(struct ch376s_Context_t *pCtx) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);

    if (NULL == hw || !device_is_ready(hw->int_gpio.port)) {
        return 0;
    }

    return gpio_pin_get_dt(&hw->int_gpio) == 0 ? 1 : 0;
}
