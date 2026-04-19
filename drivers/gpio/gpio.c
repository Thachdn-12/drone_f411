#include <stdint.h>
#include "gpio.h"

/* RCC */
#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))

/* GPIOA */
#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_ODR       (*(volatile uint32_t*)(GPIOA_BASE + 0x14))

/* GPIOC */
#define GPIOC_BASE      0x40020800
#define GPIOC_MODER     (*(volatile uint32_t*)(GPIOC_BASE + 0x00))
#define GPIOC_ODR       (*(volatile uint32_t*)(GPIOC_BASE + 0x14))

void gpio_init(void)
{
   /* Enable GPIOC clock */
    RCC_AHB1ENR |= (1 << 2);

    /* PC13 = output */
    GPIOC_MODER &= ~(3 << (13 * 2));
    GPIOC_MODER |=  (1 << (13 * 2));
}

void gpio_toggle(void)
{
    GPIOC_ODR ^= (1 << 13);
}