/**
 * @file    gpio.h
 * @brief   GPIO driver for STM32F411 (Baremetal)
 * @author  Toby
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/* GPIO Base Addresses */

#define GPIOA_BASE 0x40020000
#define GPIOB_BASE 0x40020400
#define GPIOC_BASE 0x40020800
#define GPIOD_BASE 0x40020C00
#define GPIOE_BASE 0x40021000

/* RCC Config */

#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))

/* GPIO Struct */

typedef struct
{
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;

} gpio_t;

/* GPIO Instances */

#define GPIOA ((gpio_t*)GPIOA_BASE)
#define GPIOB ((gpio_t*)GPIOB_BASE)
#define GPIOC ((gpio_t*)GPIOC_BASE)
#define GPIOD ((gpio_t*)GPIOD_BASE)
#define GPIOE ((gpio_t*)GPIOE_BASE)

/* GPIO Mode*/

typedef enum
{
    GPIO_INPUT  = 0x0,
    GPIO_OUTPUT = 0x1,
    GPIO_AF     = 0x2,
    GPIO_ANALOG = 0x3

} gpio_mode_t;

/* GPIO Output Type */

typedef enum
{
    GPIO_PUSH_PULL  = 0,
    GPIO_OPEN_DRAIN = 1

} gpio_otype_t;

/* GPIO Pull*/

typedef enum
{
    GPIO_NO_PULL = 0,
    GPIO_PULL_UP = 1,
    GPIO_PULL_DOWN = 2

} gpio_pull_t;

/* GPIO Speed*/

typedef enum
{
    GPIO_LOW_SPEED       = 0,
    GPIO_MEDIUM_SPEED    = 1,
    GPIO_FAST_SPEED      = 2,
    GPIO_HIGH_SPEED      = 3

} gpio_speed_t;

/* Function Prototypes */

void gpio_clock_enable(gpio_t *gpio);
void gpio_mode(gpio_t *gpio, uint8_t pin, gpio_mode_t mode);
void gpio_output_type(gpio_t *gpio, uint8_t pin, gpio_otype_t type);
void gpio_pull(gpio_t *gpio, uint8_t pin, gpio_pull_t pull);
void gpio_speed(gpio_t *gpio, uint8_t pin, gpio_speed_t speed);
void gpio_af(gpio_t *gpio, uint8_t pin, uint8_t af);
void gpio_write(gpio_t *gpio, uint8_t pin, uint8_t state);
void gpio_toggle(gpio_t *gpio, uint8_t pin);
uint8_t gpio_read(gpio_t *gpio, uint8_t pin);

#endif