#include "timer.h"
#include "gpio.h"
#include "system.h"
/* =========================================================
 * STM32F411 Registers
 * ========================================================= */

#define TIM2_BASE           0x40000000UL

/* RCC */

#define RCC_APB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* TIM2 */

#define TIM2_CR1            (*(volatile uint32_t*)(TIM2_BASE + 0x00))
#define TIM2_CR2            (*(volatile uint32_t*)(TIM2_BASE + 0x04))
#define TIM2_SMCR           (*(volatile uint32_t*)(TIM2_BASE + 0x08))
#define TIM2_DIER           (*(volatile uint32_t*)(TIM2_BASE + 0x0C))
#define TIM2_SR             (*(volatile uint32_t*)(TIM2_BASE + 0x10))
#define TIM2_EGR            (*(volatile uint32_t*)(TIM2_BASE + 0x14))
#define TIM2_CNT            (*(volatile uint32_t*)(TIM2_BASE + 0x24))
#define TIM2_PSC            (*(volatile uint32_t*)(TIM2_BASE + 0x28))
#define TIM2_ARR            (*(volatile uint32_t*)(TIM2_BASE + 0x2C))

/* NVIC */

#define NVIC_ISER0          (*(volatile uint32_t*)0xE000E100)
#define NVIC_ICER0          (*(volatile uint32_t*)0xE000E180)
#define NVIC_ICPR0          (*(volatile uint32_t*)0xE000E280)
#define NVIC_TIM2_PRIORITY  (*(volatile uint8_t*)(0xE000E400UL + 28U))

#define TIM2_IRQ_MASK       (1U << 28)
#define TIM_CR1_CEN         (1U << 0)
#define TIM_CR1_ARPE        (1U << 7)
#define TIM_DIER_UIE        (1U << 0)
#define TIM_SR_UIF          (1U << 0)
#define TIM_EGR_UG          (1U << 0)
#define TIMER_MAX_FREQ_HZ   10000U

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
    if ((TIM2_SR & TIM_SR_UIF) && (TIM2_DIER & TIM_DIER_UIE))
    {
        /* Status bits are cleared by writing zero; preserve other flags. */
        TIM2_SR = ~TIM_SR_UIF;

        g_timer2_tick++;
    }
}

/* =========================================================
 * Init TIM2
 * ========================================================= */

int timer_init(uint32_t freq_hz)
{
    /* APB1 is undivided in system.c, so TIM2 clock equals the core clock.
     * Revisit this calculation if APB1 prescalers are introduced.
     */
    uint32_t timer_clk = system_core_clock();

    if ((freq_hz == 0U) || (freq_hz > TIMER_MAX_FREQ_HZ) || (freq_hz > timer_clk))
    {
        return -1;
    }

    /* Enable TIM2 clock */
    RCC_APB1ENR |= (1 << 0);
    (void)RCC_APB1ENR;

    NVIC_ICER0 = TIM2_IRQ_MASK;
    TIM2_CR1 = 0;
    TIM2_DIER = 0;
    TIM2_CR2 = 0;
    TIM2_SMCR = 0;

    /* TIM2 has a 32-bit counter: no prescaler is needed for 1..10000 Hz.
     * At 16 MHz, 1000 Hz uses ARR=15999 and gives one IRQ per millisecond.
     */
    TIM2_PSC = 0;
    TIM2_ARR = (uint32_t)(((uint64_t)timer_clk + freq_hz / 2U) / freq_hz) - 1U;
    TIM2_CNT = 0;
    TIM2_CR1 = TIM_CR1_ARPE;
    TIM2_EGR = TIM_EGR_UG; /* Load PSC/ARR before the first period. */

    /* UG sets UIF: clear it and any pending IRQ before enabling delivery. */
    TIM2_SR = 0;
    NVIC_ICPR0 = TIM2_IRQ_MASK;
    g_timer2_tick = 0;
    NVIC_TIM2_PRIORITY = 0xF0U; /* Lowest priority, 4 implemented priority bits. */
    TIM2_DIER = TIM_DIER_UIE;
    NVIC_ISER0 = TIM2_IRQ_MASK;
    TIM2_CR1 |= TIM_CR1_CEN;
    return 0;
}

/* =========================================================
 * Get Tick
 * ========================================================= */

uint32_t timer_get_tick(void)
{
    return g_timer2_tick;
}
