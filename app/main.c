#include "gpio.h"
#include "systick.h"

int main(void)
{
    systick_init();
    gpio_init();

    while (1)
    {
        // 3 nháy nhanh
        for (int i = 0; i < 3; i++)
        {
            gpio_toggle();
            delay_ms(100);
        }

        // pause
        delay_ms(1000);

        // 2 nháy chậm
        for (int i = 0; i < 2; i++)
        {
            gpio_toggle();
            delay_ms(500);
        }

        // pause dài
        delay_ms(2000);
    }
}