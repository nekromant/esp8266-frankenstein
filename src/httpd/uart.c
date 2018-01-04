/* Copyright (c) 2014 Ivan Grokhotkov. All rights reserved. 
 * This file is part of the atproto AT protocol library
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */
#include "stdint.h"
#include "osapi.h"
#include "ets_sys.h"
#include "mem.h"
#include "uart.h"
#include "user_interface.h"
#include "uart_hw.h"
	
#define UART_TX_FIFO_SIZE 0x7f

struct _uart
{
    int  baud_rate;
    uart_rx_handler_t rx_handler;
};

void ICACHE_FLASH_ATTR uart0_rx_handler(uart_t* uart)
{
    if (READ_PERI_REG(UART_INT_ST(0)) & UART_RXFIFO_FULL_INT_ST)
    {
        while(true)
        {
            int rx_count = (READ_PERI_REG(UART_STATUS(0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;
            if (!rx_count)
                break;

            for(int cnt = 0; cnt < rx_count; ++cnt)
            {
                char c = READ_PERI_REG(UART_FIFO(0)) & 0xFF;
                (*uart->rx_handler)(c);
            }
        }
        WRITE_PERI_REG(UART_INT_CLR(0), UART_RXFIFO_FULL_INT_CLR);
    }
}

void ICACHE_FLASH_ATTR uart0_wait_for_tx_fifo(size_t size_needed)
{
    while (true)
    {
        size_t tx_count = (READ_PERI_REG(UART_STATUS(0)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT;
        if (tx_count <= (UART_TX_FIFO_SIZE - size_needed))
            break;
    }
}

void ICACHE_FLASH_ATTR uart0_wait_for_transmit(uart_t* uart)
{
    uart0_wait_for_tx_fifo(UART_TX_FIFO_SIZE);
}

void ICACHE_FLASH_ATTR uart0_transmit_char(uart_t* uart, char c)
{
    WRITE_PERI_REG(UART_FIFO(0), c);
}

void ICACHE_FLASH_ATTR uart0_transmit(uart_t* uart, const char* buf, int size)
{
    while (size)
    {
        size_t part_size = (size > UART_TX_FIFO_SIZE) ? UART_TX_FIFO_SIZE : size;
        size -= part_size;

        uart0_wait_for_tx_fifo(part_size);
        for(;part_size;--part_size, ++buf)
            WRITE_PERI_REG(UART_FIFO(0), *buf);
    }
}

void ICACHE_FLASH_ATTR uart0_flush(uart_t* uart)
{
    SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST | UART_TXFIFO_RST);
}

void ICACHE_FLASH_ATTR uart0_interrupt_enable(uart_t* uart)
{
    WRITE_PERI_REG(UART_INT_CLR(0), 0x1ff);
    ETS_UART_INTR_ATTACH(&uart0_rx_handler, uart);
    SET_PERI_REG_MASK(UART_INT_ENA(0), UART_RXFIFO_FULL_INT_ENA);
    ETS_UART_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR uart0_interrupt_disable(uart_t* uart)
{
    SET_PERI_REG_MASK(UART_INT_ENA(0), 0);
    ETS_UART_INTR_DISABLE();
}

void ICACHE_FLASH_ATTR uart0_set_baudrate(uart_t* uart, int baud_rate)
{
    uart->baud_rate = baud_rate;
    uart_div_modify(0, UART_CLK_FREQ / (uart->baud_rate));
}

int ICACHE_FLASH_ATTR uart0_get_baudrate(uart_t* uart)
{
    return uart->baud_rate;
}

uart_t* ICACHE_FLASH_ATTR uart0_init(int baudrate, uart_rx_handler_t rx_handler)
{
    uart_t* uart = (uart_t*) os_malloc(sizeof(uart_t));

    uart->rx_handler = rx_handler;

    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

    uart0_set_baudrate(uart, baudrate);
    WRITE_PERI_REG(UART_CONF0(0), 0x3 << UART_BIT_NUM_S);   // 8n1

    uart0_flush(uart);
    uart0_interrupt_enable(uart);
    
    WRITE_PERI_REG(UART_CONF1(0), ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));

    return uart;
}

void ICACHE_FLASH_ATTR uart0_uninit(uart_t* uart)
{
    uart0_interrupt_disable(uart);
    // TODO: revert pin functions
    os_free(uart);
}


void ICACHE_FLASH_ATTR uart_ignore_char(char c)
{    
}

void ICACHE_FLASH_ATTR uart_write_char(char c)
{
    if (c == '\n')
        WRITE_PERI_REG(UART_FIFO(0), '\r');
    
    WRITE_PERI_REG(UART_FIFO(0), c);
}

int s_uart_debug_enabled = 1;
void ICACHE_FLASH_ATTR uart_set_debug(int enabled)
{
    s_uart_debug_enabled = enabled;
    if (enabled)
        ets_install_putc1((void *)&uart_write_char);
    else
        ets_install_putc1((void *)&uart_ignore_char);
}

int ICACHE_FLASH_ATTR uart_get_debug()
{
    return s_uart_debug_enabled;
}
