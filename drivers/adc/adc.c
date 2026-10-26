#include "adc.h"
#include "gpio.h"
#include "systick.h"
#include "system.h"
/* =========================================================
 * STM32F411 Registers
 * ========================================================= */

#define ADC1_BASE           0x40012000UL

/* =========================================================
 * RCC
 * ========================================================= */

#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB2ENR         (*(volatile uint32_t*)(RCC_BASE + 0x44))

/* =========================================================
 * ADC1 Registers
 * ========================================================= */

#define ADC_SR              (*(volatile uint32_t*)(ADC1_BASE + 0x00))
#define ADC_CR1             (*(volatile uint32_t*)(ADC1_BASE + 0x04))
#define ADC_CR2             (*(volatile uint32_t*)(ADC1_BASE + 0x08))
#define ADC_SMPR2           (*(volatile uint32_t*)(ADC1_BASE + 0x10))
#define ADC_SQR1            (*(volatile uint32_t*)(ADC1_BASE + 0x2C))
#define ADC_SQR2            (*(volatile uint32_t*)(ADC1_BASE + 0x30))
#define ADC_SQR3            (*(volatile uint32_t*)(ADC1_BASE + 0x34))
#define ADC_DR              (*(volatile uint32_t*)(ADC1_BASE + 0x4C))
#define ADC_CCR             (*(volatile uint32_t*)0x40012304UL)

#define ADC_SR_EOC          (1U << 1)
#define ADC_SR_OVR          (1U << 5)
#define ADC_CR2_ADON        (1U << 0)
#define ADC_CR2_EOCS        (1U << 10)
#define ADC_CR2_SWSTART     (1U << 30)
#define ADC_CCR_ADCPRE_MASK (3U << 16)
#define ADC_CCR_ADCPRE_DIV4 (1U << 16)
#define ADC_TIMEOUT_MS      10U
#define ADC_POLL_LIMIT      1000000U

static adc_status_t adc_abort(adc_status_t error)
{
    /* Stop the conversion so it cannot supply stale data on a later read. */
    ADC_CR2 &= ~(ADC_CR2_ADON | ADC_CR2_SWSTART);
    ADC_SR = 0;
    return error;
}

/* =========================================================
 * ADC Init
 * ========================================================= */

void adc_init(void)
{
    /*
     * Enable clocks
     */

    RCC_AHB1ENR |= (1 << 0);   /* GPIOA */
    RCC_APB2ENR |= (1 << 8);   /* ADC1 */
    (void)RCC_APB2ENR;
    ADC_CR2 = 0; /* Stop any old conversion before changing configuration. */

    /* =====================================================
     * GPIO Analog Mode
     *
     * PA0 -> ADC_IN0
     * PA1 -> ADC_IN1
     * PA2 / PA3 are reserved for USART2 TX / RX.
     * ===================================================== */
    gpio_mode(GPIOA, 0, GPIO_ANALOG);
    gpio_mode(GPIOA, 1, GPIO_ANALOG);
    gpio_pull(GPIOA, 0, GPIO_NO_PULL);
    gpio_pull(GPIOA, 1, GPIO_NO_PULL);

    /* =====================================================
     * ADC Configuration
     * ===================================================== */

    /*
     * 480 cycles sample time
     * better stability
     */

    /* APB2 / 4 = 4 MHz with the current 16 MHz clock. */
    ADC_CCR = (ADC_CCR & ~ADC_CCR_ADCPRE_MASK) | ADC_CCR_ADCPRE_DIV4;
    ADC_CR1 = 0; /* 12-bit, scan/interrupts disabled. */
    ADC_SMPR2 = (7U << (0 * 3)) | (7U << (1 * 3));
    ADC_SQR1 = 0; /* Exactly one conversion per sequence. */
    ADC_SQR2 = 0;
    ADC_SQR3 = 0;
    ADC_SR = 0;

    /*
     * Enable ADC
     */

    ADC_CR2 = ADC_CR2_EOCS | ADC_CR2_ADON;

    /* Allow power-up stabilization without depending on SysTick interrupts.
     * Each loop contains at least one CPU cycle; wait at least 10 us.
     */
    uint32_t cycles = ((system_core_clock() / 1000000U) + 1U) * 10U;
    while (cycles-- > 0U)
    {
        __asm volatile ("nop");
    }
}

/* =========================================================
 * Read ADC Channel
 * ========================================================= */

adc_status_t adc_read(uint8_t channel, uint16_t *value)
{
    if ((value == 0) || ((channel != ADC_CHANNEL_0) && (channel != ADC_CHANNEL_1)))
    {
        return ADC_ERR_ARGUMENT;
    }
    if (!(ADC_CR2 & ADC_CR2_ADON))
    {
        return ADC_ERR_NOT_READY;
    }

    /* Discard old EOC/OVR flags before starting a new single conversion. */
    ADC_SR = 0;
    ADC_SQR3 = channel;
    uint32_t start = systick_get_tick();
    ADC_CR2 |= ADC_CR2_SWSTART;

    for (uint32_t polls = 0; polls < ADC_POLL_LIMIT; polls++)
    {
        uint32_t status = ADC_SR;
        if (status & ADC_SR_OVR)
        {
            return adc_abort(ADC_ERR_OVERRUN);
        }
        if (status & ADC_SR_EOC)
        {
            *value = (uint16_t)(ADC_DR & 0xFFFU);
            ADC_SR = 0;
            return ADC_OK;
        }
        if ((uint32_t)(systick_get_tick() - start) >= ADC_TIMEOUT_MS)
        {
            return adc_abort(ADC_ERR_TIMEOUT);
        }
    }
    return adc_abort(ADC_ERR_TIMEOUT);
}
