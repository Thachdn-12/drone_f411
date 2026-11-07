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

/* Foreground periodic releases in TIM2 tick units, not necessarily milliseconds.
 * Use the same start_tick for tasks that must share a phase. TIM2 must not be
 * reinitialized while schedules are active; restart the schedules if it is.
 */
typedef struct
{
    uint32_t last_tick;
    uint32_t period_ticks;
} timer_periodic_t;

/* Returns -1 for NULL/zero period, leaving existing state unchanged. */
int timer_periodic_start(timer_periodic_t *periodic, uint32_t period_ticks,
                         uint32_t start_tick);

/* Returns the number of elapsed periods and advances to the last release on
 * the original phase. Call the task ONCE even if the return value exceeds 1:
 * old releases are skipped rather than replayed. Handles uint32_t rollover
 * provided elapsed time since the last release is less than 2^32 ticks.
 * NULL/zero period returns zero.
 * Call from one foreground owner; neither this helper nor the IRQ runs tasks.
 */
uint32_t timer_periodic_poll(timer_periodic_t *periodic, uint32_t now_tick);

#endif
