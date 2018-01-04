/* Copyright (c) 2014 Ivan Grokhotkov. All rights reserved. 
 * This file is part of the atproto AT protocol library
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */

#ifndef ESP8266_UART_H
#define ESP8266_UART_H

#define UART_RX_BUFFER_SIZE 1024
#define UART_TX_BUFFER_SIZE 1024

typedef void (*uart_rx_handler_t)(char);

typedef struct _uart uart_t;

uart_t* uart0_init(int baud_rate, uart_rx_handler_t rx_handler);
void uart0_set_baudrate(uart_t* uart, int baud_rate);
int  uart0_get_baudrate(uart_t* uart);
void uart0_uninit(uart_t* uart);
void uart0_transmit(uart_t* uart, const char* buf, int size);    // may block on TX fifo
void uart0_wait_for_transmit(uart_t* uart);
void uart0_transmit_char(uart_t* uart, char c);  // does not block, but character will be lost if FIFO is full

void uart_set_debug(int enabled);
int  uart_get_debug();
#endif//ESP8266_UART_H