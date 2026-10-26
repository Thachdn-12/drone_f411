#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* =========================================================
 * APIs
 * ========================================================= */

/* TIM2 update IRQ frequency: 1..10000 Hz, rounded to the nearest timer count.
 * Return 0 on success, -1 for invalid input (running timer left unchanged).
 * A successful initialization resets the interrupt tick count to zero.
 */
int timer_init(uint32_t freq_hz);

/* Number of TIM2 update interrupts, wrapping at UINT32_MAX. */
uint32_t timer_get_tick(void);

#endif
