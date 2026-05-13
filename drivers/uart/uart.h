#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(uint32_t baudrate);

void uart_write_char(char c);
void uart_write_string(const char *s);
void uart_write_int(int value);

char uart_read_char(void);
void uart_print_uint(uint32_t value);
#endif