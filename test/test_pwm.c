#include "pwm.h"
#include "systick.h"

int main(void)
{
    systick_init();
    pwm_init();

    /* ESC arming */
    pwm_set_us(1000);
    delay_ms(3000);

    while (1)
    {
        pwm_set_us(1100);
        delay_ms(2000);

        pwm_set_us(1200);
        delay_ms(2000);
    }
}