/**
 * @file    gpio.c
 * @brief   GPIO driver for STM32F411 (Baremetal)
 * @author  Toby
 */
#include "gpio.h"

static int gpio_valid(gpio_t *gpio, uint8_t pin)
{
    return pin < 16U && (gpio == GPIOA || gpio == GPIOB || gpio == GPIOC ||
                        gpio == GPIOD || gpio == GPIOE);
}

/**
 * @brief  Enable clock for GPIOX of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @retval None
 */
void gpio_clock_enable(gpio_t *gpio)
{
    if (gpio == GPIOA)
        RCC_AHB1ENR |= (1U << 0);

    else if (gpio == GPIOB)
        RCC_AHB1ENR |= (1U << 1);

    else if (gpio == GPIOC)
        RCC_AHB1ENR |= (1U << 2);

    else if (gpio == GPIOD)
        RCC_AHB1ENR |= (1U << 3);

    else if (gpio == GPIOE)
        RCC_AHB1ENR |= (1U << 4);
    else
        return;

    (void)RCC_AHB1ENR; /* Allow the peripheral clock enable to take effect. */
}

/**
 * @brief  Setting GPIO Mode of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @param  mode: Mode of pin
 * @retval None
 */
void gpio_mode(gpio_t *gpio, uint8_t pin, gpio_mode_t mode)
{
    if (!gpio_valid(gpio, pin) || (uint32_t)mode > GPIO_ANALOG)
        return;

    gpio->MODER = (gpio->MODER & ~(3U << (pin * 2U))) |
                  ((uint32_t)mode << (pin * 2U));
}

/**
 * @brief  Setting GPIO Output type of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @param  type: Output type pin
 * @retval None
 */
void gpio_output_type(gpio_t *gpio, uint8_t pin, gpio_otype_t type)
{
    if (!gpio_valid(gpio, pin) || (uint32_t)type > GPIO_OPEN_DRAIN)
        return;

    gpio->OTYPER = (gpio->OTYPER & ~(1U << pin)) | ((uint32_t)type << pin);
}

/**
 * @brief  Setting GPIO Pull Up/ Pull Down of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @param  pull: pullup / pulldown
 * @retval None
 */
void gpio_pull(gpio_t *gpio, uint8_t pin, gpio_pull_t pull)
{
    if (!gpio_valid(gpio, pin) || (uint32_t)pull > GPIO_PULL_DOWN)
        return;

    gpio->PUPDR = (gpio->PUPDR & ~(3U << (pin * 2U))) |
                  ((uint32_t)pull << (pin * 2U));
}

/**
 * @brief  Setting GPIO speed of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @param  speed: Speed
 * @retval None
 */
void gpio_speed(gpio_t *gpio, uint8_t pin, gpio_speed_t speed)
{
    if (!gpio_valid(gpio, pin) || (uint32_t)speed > GPIO_HIGH_SPEED)
        return;

    gpio->OSPEEDR = (gpio->OSPEEDR & ~(3U << (pin * 2U))) |
                    ((uint32_t)speed << (pin * 2U));
}

/**
 * @brief  Setting GPIO alternate functon of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @param  af: alternate functon
 * @retval None
 */
void gpio_af(gpio_t *gpio, uint8_t pin, uint8_t af)
{
    if (!gpio_valid(gpio, pin) || af > 15U)
        return;

    volatile uint32_t *reg = pin < 8U ? &gpio->AFRL : &gpio->AFRH;
    uint32_t shift = (pin % 8U) * 4U;
    *reg = (*reg & ~(15U << shift)) | ((uint32_t)af << shift);
}

/**
 * @brief  Setting GPIO write pin of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @param  state: state
 * @retval None
 */
void gpio_write(gpio_t *gpio, uint8_t pin, uint8_t state)
{
    if (!gpio_valid(gpio, pin))
        return;

    if (state)
        gpio->BSRR = (1U << pin);

    else
        gpio->BSRR = (1U << (pin + 16U));
}

/**
 * @brief  Setting GPIO toggle function of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @retval None
 */
void gpio_toggle(gpio_t *gpio,
                 uint8_t pin)
{
    if (!gpio_valid(gpio, pin))
        return;

    /* Update only this output through BSRR; no read-modify-write of ODR. */
    gpio_write(gpio, pin, (gpio->ODR & (1U << pin)) == 0U);
}

/**
 * @brief  Setting GPIO read pin of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @param  pin:  Pin value
 * @returns None
 */
uint8_t gpio_read(gpio_t *gpio,
                  uint8_t pin)
{
    if (!gpio_valid(gpio, pin))
        return 0;

    return (gpio->IDR >> pin) & 1U;
}
