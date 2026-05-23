#include "adc.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"
/* =========================================================
 * STM32F411 Registers
 * ========================================================= */

#define RCC_BASE            0x40023800UL
#define GPIOA_BASE          0x40020000UL
#define ADC1_BASE           0x40012000UL

/* =========================================================
 * RCC
 * ========================================================= */

#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB2ENR         (*(volatile uint32_t*)(RCC_BASE + 0x44))

/* =========================================================
 * GPIOA
 * ========================================================= */

//#define GPIOA_MODER         (*(volatile uint32_t*)(GPIOA_BASE + 0x00))

/* =========================================================
 * ADC1 Registers
 * ========================================================= */

#define ADC_SR              (*(volatile uint32_t*)(ADC1_BASE + 0x00))
#define ADC_CR1             (*(volatile uint32_t*)(ADC1_BASE + 0x04))
#define ADC_CR2             (*(volatile uint32_t*)(ADC1_BASE + 0x08))
#define ADC_SMPR2           (*(volatile uint32_t*)(ADC1_BASE + 0x10))
#define ADC_SQR3            (*(volatile uint32_t*)(ADC1_BASE + 0x34))
#define ADC_DR              (*(volatile uint32_t*)(ADC1_BASE + 0x4C))

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

    /* =====================================================
     * GPIO Analog Mode
     *
     * PA0 -> ADC_IN0
     * PA1 -> ADC_IN1
     * PA2 -> ADC_IN2
     * PA3 -> ADC_IN3
     * ===================================================== */
    gpio_mode(GPIOA, 0, GPIO_ANALOG); // GPIOA_MODER |= (0xF << 0);
    gpio_mode(GPIOA, 1, GPIO_ANALOG); // GPIOA_MODER |= (0xF << 0);
    gpio_mode(GPIOA, 2, GPIO_ANALOG); // GPIOA_MODER |= (0xF << 0);
    gpio_mode(GPIOA, 3, GPIO_ANALOG); // GPIOA_MODER |= (0xF << 0);

    /* =====================================================
     * ADC Configuration
     * ===================================================== */

    /*
     * 480 cycles sample time
     * better stability
     */

    ADC_SMPR2 |= (7 << (0 * 3));
    ADC_SMPR2 |= (7 << (1 * 3));
    ADC_SMPR2 |= (7 << (2 * 3));
    ADC_SMPR2 |= (7 << (3 * 3));

    /*
     * Enable ADC
     */

    ADC_CR2 |= (1 << 0);
}

/* =========================================================
 * Read ADC Channel
 * ========================================================= */

uint16_t adc_read(uint8_t channel)
{
    /*
     * Select channel
     */

    ADC_SQR3 = channel;

    /*
     * Start conversion
     */

    ADC_CR2 |= (1 << 30);

    /*
     * Wait EOC
     */

    while (!(ADC_SR & (1 << 1)))
    {
    }

    /*
     * Read result
     */

    return (uint16_t)ADC_DR;
}