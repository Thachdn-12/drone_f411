/* =========================================================
 * STM32F411 USART2 Driver
 * PA9 -> TX
 * PA10 -> RX
 * ========================================================= */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/* =========================================================
 * APIs
 * ========================================================= */

void uart_init(void);

void uart_write_char(char c);

void uart_write_string(const char *str);

char uart_read_char(void);

void uart_printf(const char *fmt, ...);

#endif