/**
 * @file    gpio.c
 * @brief   GPIO driver for STM32F411 (Baremetal)
 * @author  Toby
 */
#include "gpio.h"

/**
 * @brief  Enable clock for GPIOX of STM32F411 Blackpill
 * @param  gpio: GPIO base address
 * @retval None
 */
void gpio_clock_enable(gpio_t *gpio)
{
    if (gpio == GPIOA)
        RCC_AHB1ENR |= (1 << 0);

    else if (gpio == GPIOB)
        RCC_AHB1ENR |= (1 << 1);

    else if (gpio == GPIOC)
        RCC_AHB1ENR |= (1 << 2);

    else if (gpio == GPIOD)
        RCC_AHB1ENR |= (1 << 3);

    else if (gpio == GPIOE)
        RCC_AHB1ENR |= (1 << 4);
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
    gpio->MODER &= ~(0x3 << (pin * 2));

    gpio->MODER |= (mode << (pin * 2));
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
    gpio->OTYPER &= ~(1 << pin);

    gpio->OTYPER |= (type << pin);
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
    gpio->PUPDR &= ~(0x3 << (pin * 2));

    gpio->PUPDR |= (pull << (pin * 2));
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
    gpio->OSPEEDR &= ~(0x3 << (pin * 2));

    gpio->OSPEEDR |= (speed << (pin * 2));
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
    if (pin < 8)
    {
        gpio->AFRL &= ~(0xF << (pin * 4));

        gpio->AFRL |= (af << (pin * 4));
    }
    else
    {
        pin -= 8;

        gpio->AFRH &= ~(0xF << (pin * 4));

        gpio->AFRH |= (af << (pin * 4));
    }
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
    if (state)
        gpio->BSRR = (1 << pin);

    else
        gpio->BSRR = (1 << (pin + 16));
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
    gpio->ODR ^= (1 << pin);
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
    return (gpio->IDR >> pin) & 0x1;
}