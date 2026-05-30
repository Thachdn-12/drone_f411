#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/* =========================================================
 * APIs
 * ========================================================= */

void systick_init(uint32_t core_clock_hz);
uint32_t systick_get_tick(void);
void delay_ms(uint32_t ms);

#endif