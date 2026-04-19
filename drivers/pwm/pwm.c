#include <stdint.h>

/* RCC */
#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* GPIOA */
#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL      (*(volatile uint32_t*)(GPIOA_BASE + 0x20))

/* TIM3 */
#define TIM3_BASE       0x40000400
#define TIM3_PSC        (*(volatile uint32_t*)(TIM3_BASE + 0x28))
#define TIM3_ARR        (*(volatile uint32_t*)(TIM3_BASE + 0x2C))
#define TIM3_CCR1       (*(volatile uint32_t*)(TIM3_BASE + 0x34))
#define TIM3_CCMR1      (*(volatile uint32_t*)(TIM3_BASE + 0x18))
#define TIM3_CCER       (*(volatile uint32_t*)(TIM3_BASE + 0x20))
#define TIM3_CR1        (*(volatile uint32_t*)(TIM3_BASE + 0x00))

void pwm_init(void)
{
    /* Enable clock */
    RCC_AHB1ENR |= (1 << 0);   // GPIOA
    RCC_APB1ENR |= (1 << 1);   // TIM3

    /* PA6 → AF2 (TIM3_CH1) */
    GPIOA_MODER &= ~(3 << (6 * 2));
    GPIOA_MODER |=  (2 << (6 * 2));

    GPIOA_AFRL &= ~(0xF << (6 * 4));
    GPIOA_AFRL |=  (2 << (6 * 4));

    /* Timer config */
    TIM3_PSC = 16 - 1;      // 1MHz
    TIM3_ARR = 20000 - 1;   // 20ms

    /* PWM mode 1 */
    TIM3_CCMR1 |= (6 << 4);   // OC1M = PWM mode 1
    TIM3_CCMR1 |= (1 << 3);   // preload enable

    TIM3_CCER |= (1 << 0);    // enable CH1

    TIM3_CCR1 = 1000;         // start at 1000us

    TIM3_CR1 |= (1 << 0);     // enable timer
}

void pwm_set_us(uint16_t us)
{
    if (us < 1000) us = 1000;
    if (us > 2000) us = 2000;

    TIM3_CCR1 = us;
}