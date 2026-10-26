#include "pwm.h"
#include "gpio.h"
#include "system.h"

/* =========================================================
 * STM32F411 Registers
 * ========================================================= */

#define TIM1_BASE           0x40010000UL

/* =========================================================
 * RCC
 * ========================================================= */

#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB2ENR         (*(volatile uint32_t*)(RCC_BASE + 0x44))

/* =========================================================
 * GPIOA
 * PA8  -> TIM1_CH1
 * PA9  -> TIM1_CH2
 * PA10 -> TIM1_CH3
 * PA11 -> TIM1_CH4
 * ========================================================= */

/* =========================================================
 * TIM1
 * ========================================================= */

#define TIM1_CR1            (*(volatile uint32_t*)(TIM1_BASE + 0x00))
#define TIM1_CR2            (*(volatile uint32_t*)(TIM1_BASE + 0x04))
#define TIM1_SMCR           (*(volatile uint32_t*)(TIM1_BASE + 0x08))
#define TIM1_DIER           (*(volatile uint32_t*)(TIM1_BASE + 0x0C))
#define TIM1_SR             (*(volatile uint32_t*)(TIM1_BASE + 0x10))
#define TIM1_CCMR1          (*(volatile uint32_t*)(TIM1_BASE + 0x18))
#define TIM1_CCMR2          (*(volatile uint32_t*)(TIM1_BASE + 0x1C))
#define TIM1_CCER           (*(volatile uint32_t*)(TIM1_BASE + 0x20))
#define TIM1_CNT            (*(volatile uint32_t*)(TIM1_BASE + 0x24))
#define TIM1_PSC            (*(volatile uint32_t*)(TIM1_BASE + 0x28))
#define TIM1_ARR            (*(volatile uint32_t*)(TIM1_BASE + 0x2C))
#define TIM1_RCR            (*(volatile uint32_t*)(TIM1_BASE + 0x30))
#define TIM1_CCR1           (*(volatile uint32_t*)(TIM1_BASE + 0x34))
#define TIM1_CCR2           (*(volatile uint32_t*)(TIM1_BASE + 0x38))
#define TIM1_CCR3           (*(volatile uint32_t*)(TIM1_BASE + 0x3C))
#define TIM1_CCR4           (*(volatile uint32_t*)(TIM1_BASE + 0x40))
#define TIM1_BDTR           (*(volatile uint32_t*)(TIM1_BASE + 0x44))
#define TIM1_EGR            (*(volatile uint32_t*)(TIM1_BASE + 0x14))

#define PWM_COUNTER_HZ      1000000UL
#define PWM_PERIOD_US       20000U
#define PWM_MIN_US          1000U
#define PWM_MAX_US          2000U

#define TIM_CR1_CEN         (1U << 0)
#define TIM_CR1_ARPE        (1U << 7)
#define TIM_BDTR_MOE        (1U << 15)
#define TIM_EGR_UG          (1U << 0)

/* =========================================================
 * PWM Init
 * ========================================================= */

void pwm_init(void)
{
    RCC_AHB1ENR |= (1 << 0);   /* GPIOA */
    RCC_APB2ENR |= (1 << 0);   /* TIM1 */
    (void)RCC_APB2ENR;

    /* Stop outputs while configuring, including repeated initialization. */
    TIM1_CR1 = 0;
    TIM1_CCER = 0;
    TIM1_BDTR = 0;
    TIM1_CR2 = 0;
    TIM1_SMCR = 0;
    TIM1_DIER = 0;
    TIM1_RCR = 0;

    /* PA8..PA11: AF1, push-pull, active-high TIM1_CH1..CH4. */
    for (uint8_t pin = 8; pin <= 11; pin++)
    {
        gpio_af(GPIOA, pin, 1);
        gpio_output_type(GPIOA, pin, GPIO_PUSH_PULL);
        gpio_pull(GPIOA, pin, GPIO_NO_PULL);
        gpio_speed(GPIOA, pin, GPIO_HIGH_SPEED);
        gpio_mode(GPIOA, pin, GPIO_AF);
    }

    /* APB2 is undivided in system.c, so TIM1 clock equals the core clock.
     * 16 MHz / (15 + 1) = 1 MHz: each count is 1 us.
     * Revisit the clock calculation if APB2 prescalers are introduced.
     */
    TIM1_PSC = (system_core_clock() / PWM_COUNTER_HZ) - 1U;
    TIM1_ARR = PWM_PERIOD_US - 1U; /* 20 ms period = 50 Hz. */

    /* PWM mode 1, output compare preload on all four channels. */
    TIM1_CCMR1 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11);
    TIM1_CCMR2 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11);
    TIM1_CCR1 = PWM_MIN_US;
    TIM1_CCR2 = PWM_MIN_US;
    TIM1_CCR3 = PWM_MIN_US;
    TIM1_CCR4 = PWM_MIN_US;
    TIM1_CNT = 0;

    /* Load PSC/ARR/CCR before starting the first frame. */
    TIM1_CR1 = TIM_CR1_ARPE;
    TIM1_EGR = TIM_EGR_UG;
    TIM1_SR = 0;
    TIM1_CCER = (1U << 0) | (1U << 4) | (1U << 8) | (1U << 12);
    TIM1_BDTR = TIM_BDTR_MOE;
    TIM1_CR1 |= TIM_CR1_CEN;
}

/* =========================================================
 * Set PWM Pulse Width
 * ========================================================= */

void pwm_set_us(uint8_t channel,
                uint16_t pulse_us)
{
    /*
     * ESC standard:
     * 1000us -> 2000us
     */

    if (pulse_us < PWM_MIN_US)
    {
        pulse_us = PWM_MIN_US;
    }

    if (pulse_us > PWM_MAX_US)
    {
        pulse_us = PWM_MAX_US;
    }

    switch (channel)
    {
        case PWM_CH1:
            TIM1_CCR1 = pulse_us;
            break;

        case PWM_CH2:
            TIM1_CCR2 = pulse_us;
            break;

        case PWM_CH3:
            TIM1_CCR3 = pulse_us;
            break;

        case PWM_CH4:
            TIM1_CCR4 = pulse_us;
            break;

        default:
            break;
    }
}
