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

/* Pins are numbered 0..15 (not bit masks), on GPIOA..GPIOE.
 * Invalid ports, pins or configuration values are ignored; invalid reads return 0.
 * Call gpio_clock_enable() before accessing a port. Serialize configuration
 * changes to the same port; serialize toggles of the same pin.
 */
void gpio_clock_enable(gpio_t *gpio);
void gpio_mode(gpio_t *gpio, uint8_t pin, gpio_mode_t mode);
void gpio_output_type(gpio_t *gpio, uint8_t pin, gpio_otype_t type);
void gpio_pull(gpio_t *gpio, uint8_t pin, gpio_pull_t pull);
void gpio_speed(gpio_t *gpio, uint8_t pin, gpio_speed_t speed);
void gpio_af(gpio_t *gpio, uint8_t pin, uint8_t af);
void gpio_write(gpio_t *gpio, uint8_t pin, uint8_t state);
void gpio_toggle(gpio_t *gpio, uint8_t pin);
uint8_t gpio_read(gpio_t *gpio, uint8_t pin);

typedef enum
{
    GPIO_EXTI_RISING = 1,
    GPIO_EXTI_FALLING = 2,
    GPIO_EXTI_BOTH = 3
} gpio_exti_trigger_t;

typedef enum
{
    GPIO_EXTI_OK = 0,
    GPIO_EXTI_ERR_ARGUMENT = -1,
    GPIO_EXTI_ERR_BUSY = -2,
    GPIO_EXTI_ERR_NOT_ATTACHED = -3
} gpio_exti_status_t;

typedef void (*gpio_exti_callback_t)(gpio_t *gpio, uint8_t pin, void *context);

/* Enables the GPIO/SYSCFG clocks, configures input/pull/edge and enables NVIC.
 * Only one port can own EXTI line N at a time: PA0 and PB0 conflict.
 * Attaching the same port/pin again replaces its configuration and callback.
 * Call attach from main/thread context. The driver owns GPIO EXTI lines 0..15.
 * IRQ groups use fixed priority 15 (lowest); callbacks run in interrupt context
 * and must not block. No debounce is performed. context may be NULL and must
 * remain valid until detached. Configuration updates preserve PRIMASK.
 */
gpio_exti_status_t gpio_exti_attach(gpio_t *gpio, uint8_t pin,
                                    gpio_pull_t pull, gpio_exti_trigger_t trigger,
                                    gpio_exti_callback_t callback, void *context);

/* Removes only the matching port/pin; preserves other lines sharing its IRQ.
 * Leaves GPIO mode/pull unchanged. Also callable from a callback.
 */
gpio_exti_status_t gpio_exti_detach(gpio_t *gpio, uint8_t pin);

#endif
