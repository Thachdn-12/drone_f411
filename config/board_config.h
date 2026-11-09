/* PB0 to GND enables bench tests; open/missing jumper is pulled high (locked).
 * No EXTI is used. Read by the foreground GCS task at least every 10 ms. */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include "gpio.h"
#define BOARD_BENCH_PORT GPIOB
#define BOARD_BENCH_PIN  0U
#endif
