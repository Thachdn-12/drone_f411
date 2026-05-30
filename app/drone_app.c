#include "uart.h"
#include "gpio.h"
#include "systick.h"
#include "adc.h"
#include "pwm.h"
#include "mpu6050.h"
#include "task_imu.h"

/* =========================================================
 * App Run
 * ========================================================= */

void drone_app_run(void)
{
    uart_printf("Starting Drone App\r\n");
    while (1)
    {
        task_imu_run();

        const imu_state_t *imu = task_imu_get();

        uart_printf(
            "R=%d P=%d\r\n",
            (int)imu->roll,
            (int)imu->pitch
        );

        delay_ms(100);
    }
}
