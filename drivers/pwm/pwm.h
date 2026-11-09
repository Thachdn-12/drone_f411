#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <stdbool.h>

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

/* TIM1 CH1..CH4 on PA8..PA11, 50 Hz. Outputs stay disabled after init. */
void pwm_init(void);

/* Clamp to 1000..2000 us; preload only, never enables an output. */
void pwm_set_us(uint8_t channel,
                uint16_t pulse_us);

/* Foreground-only: validate, write compare register and enable just this channel.
 * Subsequent changes take effect at the next 50 Hz update. A true return means
 * driver/register acceptance, not a measured waveform or ESC acknowledgement. */
bool pwm_write_us(uint8_t channel, uint16_t pulse_us);
bool pwm_disable_outputs(void);

#endif
