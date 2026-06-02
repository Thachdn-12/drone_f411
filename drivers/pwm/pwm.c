#include "pwm.h"
#include "gpio.h"
#include "timer.h"
#include "systick.h"
#include "uart.h"

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
#define TIM1_CCMR1          (*(volatile uint32_t*)(TIM1_BASE + 0x18))
#define TIM1_CCMR2          (*(volatile uint32_t*)(TIM1_BASE + 0x1C))
#define TIM1_CCER           (*(volatile uint32_t*)(TIM1_BASE + 0x20))
#define TIM1_PSC            (*(volatile uint32_t*)(TIM1_BASE + 0x28))
#define TIM1_ARR            (*(volatile uint32_t*)(TIM1_BASE + 0x2C))
#define TIM1_CCR1           (*(volatile uint32_t*)(TIM1_BASE + 0x34))
#define TIM1_CCR2           (*(volatile uint32_t*)(TIM1_BASE + 0x38))
#define TIM1_CCR3           (*(volatile uint32_t*)(TIM1_BASE + 0x3C))
#define TIM1_CCR4           (*(volatile uint32_t*)(TIM1_BASE + 0x40))
#define TIM1_BDTR           (*(volatile uint32_t*)(TIM1_BASE + 0x44))
#define TIM1_EGR            (*(volatile uint32_t*)(TIM1_BASE + 0x14))

/* =========================================================
 * PWM Init
 * ========================================================= */

void pwm_init(void)
{
    /*
     * Enable clocks
     */

    RCC_AHB1ENR |= (1 << 0);   /* GPIOA */
    RCC_APB2ENR |= (1 << 0);   /* TIM1 */

    /* =====================================================
     * GPIO Alternate Function
     * ===================================================== */

    /*
     * PA8  -> AF1 TIM1_CH1
     * PA9  -> AF1 TIM1_CH2
     * PA10 -> AF1 TIM1_CH3
     * PA11 -> AF1 TIM1_CH4
     */

    /* MODER = Alternate Function */
    gpio_mode(GPIOA, 8, GPIO_AF);       // GPIOA_MODER |=  (0xAA << 16);
    //gpio_mode(GPIOA, 9, GPIO_AF);       // GPIOA_MODER |=  (0xAA << 16);
    //gpio_mode(GPIOA, 10, GPIO_AF);      // GPIOA_MODER |=  (0xAA << 16);
    gpio_mode(GPIOA, 11, GPIO_AF) ;     // GPIOA_MODER |=  (0xAA << 16);

    /* High speed */
    gpio_speed(GPIOA, 8, GPIO_HIGH_SPEED);
    //gpio_speed(GPIOA, 9, GPIO_HIGH_SPEED);
    //gpio_speed(GPIOA, 10, GPIO_HIGH_SPEED);
    gpio_speed(GPIOA, 11, GPIO_HIGH_SPEED);

    /* AFRH AF1 */
    gpio_af(GPIOA, 8, 0x1);
    //gpio_af(GPIOA, 9, 0x1);
    //gpio_af(GPIOA, 10, 0x1);
    gpio_af(GPIOA, 11, 0x1);

    /* =====================================================
     * TIM1 Configuration
     * ===================================================== */

    /*
     * Timer clock:
     * 100 MHz
     *
     * Prescaler:
     * 100MHz / 100 = 1MHz
     *
     * => 1 tick = 1us
     */

    TIM1_PSC = 100 - 1;

    /*
     * PWM period:
     *
     * 20ms = 20000us
     *
     * => 50Hz
     */

    TIM1_ARR = 20000 - 1;
  
    /* =====================================================
     * PWM Mode 1
     * OCxM = 110
     * OCxPE = 1
     * ===================================================== */

    /* CH1 + CH2 */
    TIM1_CCMR1 |= (6 << 4);
    TIM1_CCMR1 |= (1 << 3);
    
    TIM1_CCMR1 |= (6 << 12);
    TIM1_CCMR1 |= (1 << 11);

    /* CH3 + CH4 */
    TIM1_CCMR2 |= (6 << 4);
    TIM1_CCMR2 |= (1 << 3);

    TIM1_CCMR2 |= (6 << 12);
    TIM1_CCMR2 |= (1 << 11);

    /* =====================================================
     * Enable channels
     * ===================================================== */

    TIM1_CCER |= (1 << 0);
    TIM1_CCER |= (1 << 4);
    TIM1_CCER |= (1 << 8);
    TIM1_CCER |= (1 << 12);

     /* =====================================================
     * Initial pulse width
     * ESC safe minimum
     * ===================================================== */
    /* =====================================================
     * Initial pulse width
     * ESC safe minimum
     * ===================================================== */

    TIM1_CCR1 = 1000;
    TIM1_CCR2 = 1000;
    TIM1_CCR3 = 1000;
    TIM1_CCR4 = 1000;

    /* =====================================================
     * Advanced timer main output enable
     * ===================================================== */

    TIM1_BDTR |= (1 << 15);

    /* Update registers */
    TIM1_EGR |= (1 << 0);

    /* Enable counter */
    TIM1_CR1 |= (1 << 0);
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

    if (pulse_us < 1000)
    {
        pulse_us = 1000;
    }

    if (pulse_us > 2000)
    {
        pulse_us = 2000;
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