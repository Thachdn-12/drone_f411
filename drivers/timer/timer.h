#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* =========================================================
 * APIs
 * ========================================================= */

void timer_init(uint32_t freq_hz);

uint32_t timer_get_tick(void);

#endif