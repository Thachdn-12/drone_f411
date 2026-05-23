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

void pwm_init(void);

void pwm_set_us(uint8_t channel,
                uint16_t pulse_us);

#endif