#include "gpio.h"
#include "systick.h"
#include "system.h"

int main(void)
{
    system_init();
    systick_init();
    gpio_init();

    while (1)
    {
        gpio_toggle();
        delay_ms(500);
    }
}