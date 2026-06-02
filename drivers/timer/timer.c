#include "timer.h"
#include "gpio.h"
/* =========================================================
 * STM32F411 Registers
 * ========================================================= */

#define TIM2_BASE           0x40000000UL

/* RCC */

#define RCC_APB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* TIM2 */

#define TIM2_CR1            (*(volatile uint32_t*)(TIM2_BASE + 0x00))
#define TIM2_DIER           (*(volatile uint32_t*)(TIM2_BASE + 0x0C))
#define TIM2_SR             (*(volatile uint32_t*)(TIM2_BASE + 0x10))
#define TIM2_CNT            (*(volatile uint32_t*)(TIM2_BASE + 0x24))
#define TIM2_PSC            (*(volatile uint32_t*)(TIM2_BASE + 0x28))
#define TIM2_ARR            (*(volatile uint32_t*)(TIM2_BASE + 0x2C))

/* NVIC */

#define NVIC_ISER0          (*(volatile uint32_t*)0xE000E100)

/* =========================================================
 * Globals
 * ========================================================= */

static volatile uint32_t g_timer2_tick = 0;

/* =========================================================
 * TIM2 IRQ Handler
 * ========================================================= */

void TIM2_IRQHandler(void)
{
    /* Update interrupt */
    if (TIM2_SR & (1 << 0))
    {
        TIM2_SR &= ~(1 << 0);

        g_timer2_tick++;
    }
}

/* =========================================================
 * Init TIM2
 * ========================================================= */

void timer_init(uint32_t freq_hz)
{
    /*
     * STM32F411
     * APB1 Timer Clock = 100 MHz
     */

    uint32_t timer_clk = 100000000UL;

    uint32_t prescaler;
    uint32_t arr;

    /* Enable TIM2 clock */
    RCC_APB1ENR |= (1 << 0);

    /*
     * Timer tick = 10 kHz
     */

    prescaler = (timer_clk / 10000UL) - 1;

    /*
     * Generate interrupt at freq_hz
     */

    arr = (10000UL / freq_hz) - 1;

    TIM2_PSC = prescaler;
    TIM2_ARR = arr;

    /* Enable update interrupt */
    TIM2_DIER |= (1 << 0);

    /* Enable TIM2 IRQ in NVIC */
    NVIC_ISER0 |= (1 << 28);

    /* Enable counter */
    TIM2_CR1 |= (1 << 0);
}

/* =========================================================
 * Get Tick
 * ========================================================= */

uint32_t timer_get_tick(void)
{
    return g_timer2_tick;
}
