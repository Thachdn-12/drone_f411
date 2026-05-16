#include <stdint.h>

#include "i2c.h"
#include "gpio.h"

volatile uint8_t who_am_i = 0;

int main(void)
{
    gpio_clock_enable(GPIOB);
    gpio_mode(GPIOB,6,GPIO_OUTPUT);
    gpio_mode(GPIOB,7,GPIO_OUTPUT);

    while (1)
    {
        gpio_toggle(GPIOB,6);
        gpio_toggle(GPIOB,7);
        for (volatile int i = 0; i < 100000; i++);
    }
}