#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/* =========================================================
 * PWM Channels
 * ========================================================= */

#define PWM_CH1    1
#define PWM_CH2    2
#define PWM_CH3    3
#define PWM_CH4    4

/* =========================================================
 * APIs
 * ========================================================= */

/* TIM1 CH1..CH4 on PA8..PA11, 50 Hz, initially 1000 us per channel. */
void pwm_init(void);

/* Clamp to 1000..2000 us; apply on the next PWM update. Invalid channels ignored. */
void pwm_set_us(uint8_t channel,
                uint16_t pulse_us);

#endif
