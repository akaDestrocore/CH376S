/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file           ch376s_uart_rp2.c
 * @brief          CH376S UART hardware interface for RP2040/RP2350
 * @details        Uses standard 8-bit PIO UART programs from Pico SDK
 */

#include "ch376s_uart.h"

LOG_MODULE_DECLARE(ch376s_uart);

/* --------------------------------------------------------------------------
 * PIO Programs (8-bit UART from Pico SDK)
 * -------------------------------------------------------------------------*/

// TX Program
#define uart_tx_wrap_target 0
#define uart_tx_wrap 3
#define uart_tx_offset_start 0
#define uart_tx_offset_pull 0
#define uart_tx_offset_bitloop 1

static const uint16_t uart_tx_program_instructions[] = {
    0x9080, //  0: pull   block           side 1 [7]
    0xf027, //  1: set    x, 7            side 0 [7]
    0x6001, //  2: out    pins, 1
    0x0642, //  3: jmp    x--, 2          [6]
};

static const struct pio_program uart_tx_program = {
    .instructions = uart_tx_program_instructions,
    .length = 4,
    .origin = -1,
};

static inline pio_sm_config uart_tx_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + uart_tx_wrap_target, offset + uart_tx_wrap);
    sm_config_set_sideset(&c, 1, true, false);
    return c;
}

static inline void uart_tx_program_init(PIO pio, uint sm, uint offset, uint pin_tx, uint baud) {
    pio_sm_set_pins_with_mask64(pio, sm, 1ull << pin_tx, 1ull << pin_tx);
    pio_sm_set_pindirs_with_mask64(pio, sm, 1ull << pin_tx, 1ull << pin_tx);
    pio_gpio_init(pio, pin_tx);

    pio_sm_config c = uart_tx_program_get_default_config(offset);
    sm_config_set_out_shift(&c, true, false, 32);
    sm_config_set_out_pins(&c, pin_tx, 1);
    sm_config_set_sideset_pins(&c, pin_tx);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    float div = (float)clock_get_hz(clk_sys) / (8 * baud);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// RX Program
#define uart_rx_mini_wrap_target 0
#define uart_rx_mini_wrap 2

static const uint16_t uart_rx_mini_program_instructions[] = {
    0x2020, //  0: wait   0 pin, 0
    0xea27, //  1: set    x, 7            [10]
    0x4001, //  2: in     pins, 1
    0x0642, //  3: jmp    x--, 2          [6]
};

static const struct pio_program uart_rx_mini_program = {
    .instructions = uart_rx_mini_program_instructions,
    .length = 4,
    .origin = -1,
};

static inline pio_sm_config uart_rx_mini_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + uart_rx_mini_wrap_target, offset + uart_rx_mini_wrap);
    return c;
}

static inline void uart_rx_mini_program_init(PIO pio, uint sm, uint offset, uint pin, uint baud) {
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    pio_gpio_init(pio, pin);
    gpio_pull_up(pin);

    pio_sm_config c = uart_rx_mini_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin);
    sm_config_set_in_shift(&c, true, true, 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    float div = (float)clock_get_hz(clk_sys) / (8 * baud);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

/* Private function prototypes -----------------------------------------------*/
static int load_pio_programs(ch376s_HwContext_t *hw);
static int init_gpio_sequence(ch376s_HwContext_t *hw);
static int configure_state_machines(ch376s_HwContext_t *hw, uint32_t baudrate);
static void flush_startup_transients(ch376s_HwContext_t *hw);

static int ch376s_write_byte_cb(struct ch376s_Context_t *pCtx, uint8_t byte);
static int ch376s_read_byte_cb(struct ch376s_Context_t *pCtx, uint8_t *byte);
static int ch376s_query_int_cb(struct ch376s_Context_t *pCtx);

static int pio_write_byte(ch376s_HwContext_t *hw, uint8_t byte);
static int pio_read_byte(ch376s_HwContext_t *hw, uint8_t *byte, k_timeout_t timeout);

/**
 * @brief CH376S UART hardware functions
 */

int ch376s_rp2_hw_init(const char *name, int uart_idx, const struct gpio_dt_spec *int_gpio, 
                       uint32_t baudrate, struct ch376s_Context_t **ppCtxOut) {
    int ret = -1;
    ch376s_HwContext_t *hw = NULL;
    struct ch376s_Context_t *pCtx = NULL;

    if (CH376S_A_USART_INDEX != uart_idx && CH376S_B_USART_INDEX != uart_idx) {
        LOG_ERR("Invalid UART index: %d (must be 0 or 1)", uart_idx);
        return -EINVAL;
    }

    hw = k_malloc(sizeof(ch376s_HwContext_t));
    if (NULL == hw) {
        LOG_ERR("Failed to allocate hardware context");
        return -ENOMEM;
    }
    memset(hw, 0x00, sizeof(ch376s_HwContext_t));

    hw->name = name;
    hw->baudrate = baudrate;

    if (CH376S_A_USART_INDEX == uart_idx) {
        hw->pio = pio0;
        hw->tx_pin = PIO_UART_TX_PIN_CH376S_A;
        hw->rx_pin = PIO_UART_RX_PIN_CH376S_A;
        hw->sm_tx = PIO_UART_SM_TX;
        hw->sm_rx = PIO_UART_SM_RX;
    } else {
        hw->pio = pio1;
        hw->tx_pin = PIO_UART_TX_PIN_CH376S_B;
        hw->rx_pin = PIO_UART_RX_PIN_CH376S_B;
        hw->sm_tx = PIO_UART_SM_TX;
        hw->sm_rx = PIO_UART_SM_RX;
    }

    if (NULL != int_gpio) {
        hw->int_gpio = *int_gpio;
    } else {
        memset(&hw->int_gpio, 0x00, sizeof(hw->int_gpio));
    }

    ret = load_pio_programs(hw);
    if (ret < 0) {
        LOG_ERR("%s: Failed to load PIO programs: %d", name, ret);
        k_free(hw);
        return ret;
    }

    ret = init_gpio_sequence(hw);
    if (ret < 0) {
        LOG_ERR("%s: GPIO init failed: %d", name, ret);
        k_free(hw);
        return ret;
    }

    ret = configure_state_machines(hw, baudrate);
    if (ret < 0) {
        LOG_ERR("%s: SM config failed: %d", name, ret);
        k_free(hw);
        return ret;
    }

    flush_startup_transients(hw);

    ret = ch376s_openContext(&pCtx, ch376s_write_byte_cb, ch376s_read_byte_cb, 
                             ch376s_query_int_cb, hw);
    if (CH376S_SUCCESS != ret) {
        LOG_ERR("%s: ch376s_openContext failed: %d", name, ret);
        k_free(hw);
        return -EIO;
    }

    *ppCtxOut = pCtx;
    LOG_INF("%s: RP2 PIO 8-bit UART initialized", name);
    return 0;
}

int ch376s_rp2_set_baudrate(struct ch376s_Context_t *pCtx, uint32_t baudrate) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);
    
    if (NULL == hw) {
        return -EINVAL;
    }
    
    pio_sm_set_enabled(hw->pio, hw->sm_tx, false);
    pio_sm_set_enabled(hw->pio, hw->sm_rx, false);
    k_msleep(10);
    
    pio_sm_clear_fifos(hw->pio, hw->sm_tx);
    pio_sm_clear_fifos(hw->pio, hw->sm_rx);
    
    int ret = configure_state_machines(hw, baudrate);
    if (ret < 0) {
        LOG_ERR("%s: Failed to reconfigure for new baudrate", hw->name);
        return ret;
    }
    
    hw->baudrate = baudrate;
    flush_startup_transients(hw);
    return 0;
}

/* --------------------------------------------------------------------------
 * Private Helper Functions
 * -------------------------------------------------------------------------*/

static int load_pio_programs(ch376s_HwContext_t *hw) {
    if (true != pio_can_add_program(hw->pio, &uart_tx_program)) {
        LOG_ERR("%s: No space for TX program", hw->name);
        return -ENOMEM;
    }

    if (true != pio_can_add_program(hw->pio, &uart_rx_mini_program)) {
        LOG_ERR("%s: No space for RX program", hw->name);
        return -ENOMEM;
    }

    hw->offset_tx = pio_add_program(hw->pio, &uart_tx_program);
    hw->offset_rx = pio_add_program(hw->pio, &uart_rx_mini_program);

    pio_sm_claim(hw->pio, hw->sm_tx);
    pio_sm_claim(hw->pio, hw->sm_rx);

    return 0;
}

static int init_gpio_sequence(ch376s_HwContext_t *hw) {
    gpio_init(hw->tx_pin);
    gpio_set_dir(hw->tx_pin, GPIO_OUT);
    gpio_put(hw->tx_pin, 1);

    gpio_init(hw->rx_pin);
    gpio_set_dir(hw->rx_pin, GPIO_IN);
    gpio_pull_up(hw->rx_pin);

    k_msleep(5);

    pio_gpio_init(hw->pio, hw->tx_pin);
    pio_gpio_init(hw->pio, hw->rx_pin);
    pio_sm_set_consecutive_pindirs(hw->pio, hw->sm_tx, hw->tx_pin, 1, true);

    return 0;
}

static int configure_state_machines(ch376s_HwContext_t *hw, uint32_t baudrate) {
    uart_rx_mini_program_init(hw->pio, hw->sm_rx, hw->offset_rx, hw->rx_pin, baudrate);
    k_busy_wait(100);

    uart_tx_program_init(hw->pio, hw->sm_tx, hw->offset_tx, hw->tx_pin, baudrate);
    return 0;
}

static void flush_startup_transients(ch376s_HwContext_t *hw) {
    pio_sm_put_blocking(hw->pio, hw->sm_tx, 0xFFu);
    k_msleep(5);
    
    for (int i = 0; i < 4; i++) {
        pio_sm_clear_fifos(hw->pio, hw->sm_rx);
        k_msleep(5);
    }

    k_msleep(100);
}

static int pio_write_byte(ch376s_HwContext_t *hw, uint8_t byte) {
    int64_t start = k_uptime_get();
    while (pio_sm_is_tx_fifo_full(hw->pio, hw->sm_tx)) {
        if ((k_uptime_get() - start) > 100) {
            LOG_ERR("%s: TX FIFO full timeout", hw->name);
            return -ETIMEDOUT;
        }
        k_busy_wait(10);
    }
    
    pio_sm_put_blocking(hw->pio, hw->sm_tx, (uint32_t)byte);
    k_busy_wait(100);
    return 0;
}

static int pio_read_byte(ch376s_HwContext_t *hw, uint8_t *byte, k_timeout_t timeout) {
    if (NULL == byte) {
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
    *byte = (uint8_t)(raw & 0xFFu);
    
    return 0;
}

/* --------------------------------------------------------------------------
 * CH376S Callback functions
 * -------------------------------------------------------------------------*/

static int ch376s_write_byte_cb(struct ch376s_Context_t *pCtx, uint8_t byte) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);

    if (!hw) {
        return CH376S_ERROR;
    }

    int ret = pio_write_byte(hw, byte);
    if (ret < 0) {
        LOG_ERR("%s: Byte write failed: %d", hw->name, ret);
        return CH376S_ERROR;
    }

    return CH376S_SUCCESS;
}

static int ch376s_read_byte_cb(struct ch376s_Context_t *pCtx, uint8_t *byte) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);
    
    if (!hw || !byte) {
        return CH376S_ERROR;
    }
    
    int ret = pio_read_byte(hw, byte, K_MSEC(50));
    
    if (ret < 0) {
        if (ret == -ETIMEDOUT) {
            return CH376S_TIMEOUT;
        }
        LOG_ERR("%s: Read failed: %d", hw->name, ret);
        return CH376S_ERROR;
    }
    
    return CH376S_SUCCESS;
}

static int ch376s_query_int_cb(struct ch376s_Context_t *pCtx) {
    ch376s_HwContext_t *hw = (ch376s_HwContext_t *)ch376s_getPriv(pCtx);

    if (NULL == hw || !device_is_ready(hw->int_gpio.port)) {
        return 0;
    }

    return gpio_pin_get_dt(&hw->int_gpio) == 0 ? 1 : 0;
}