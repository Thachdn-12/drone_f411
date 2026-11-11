/* PB0 to GND enables bench tests; open/missing jumper is pulled high (locked).
 * No EXTI is used. Read by the foreground GCS task at least every 10 ms. */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include "gpio.h"
/* Calibration for this board, measured against host time and 115200-baud RX.
 * Factory trim 16 ran at about 15.17 MHz; trim 31 gives about 16 MHz.
 * Re-measure for a different MCU/board; this is not a universal HSI trim.
 * Apply before UART, SysTick and PWM derive their timing from 16 MHz. */
#ifndef BOARD_HSI_TRIM
#define BOARD_HSI_TRIM 31U
#endif
#if BOARD_HSI_TRIM > 31U
#error "BOARD_HSI_TRIM must be in the range 0..31"
#endif
#define BOARD_BENCH_PORT GPIOB
#define BOARD_BENCH_PIN  0U
#endif
