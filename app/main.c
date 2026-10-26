#include "systick.h"
#include "uart.h"
#include "system.h"
#include "gpio.h"
#include "i2c.h"
#include "pwm.h"
#include "adc.h"
#include "mpu6050.h"
#include "task_imu.h"

/* =========================================================
 * App Entry
 * ========================================================= */

void drone_app_run(void);

/* =========================================================
 * Main
 * ========================================================= */

int main(void)
{
    /* =====================================================
     * Core Drivers
     * ===================================================== */
    
    uart_init();
    /* Print before interrupts, delays or sensor access can block boot. */
    uart_printf("\r\n====================================\r\n");
    uart_printf(" _____ ___  ____  __   __\r\n");
    uart_printf("████████╗ ██████╗ ██████╗ ██╗   ██╗\r\n");
    uart_printf("╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝\r\n");
    uart_printf("   ██║   ██║   ██║██████╔╝ ╚████╔╝ \r\n");
    uart_printf("   ██║   ██║   ██║██╔══██╗  ╚██╔╝  \r\n");
    uart_printf("   ██║   ╚██████╔╝██████╔╝   ██║    \r\n");
    uart_printf("   ╚═╝    ╚═════╝ ╚═════╝    ╚═╝    \r\n");
    uart_printf("====================================\r\n");
    uart_printf("Drone F411 - USART2 PA2/PA3 115200 8N1\r\n");
    uart_printf("Build: %s %s\r\n", __DATE__, __TIME__);
    uart_printf("Core clock: %u Hz\r\n", system_core_clock());
    uart_printf("====================================\r\n");
    systick_init(system_core_clock());

    /* =====================================================
     * Peripheral Drivers
     * ===================================================== */

    i2c_init();
    pwm_init();
    adc_init();
    uart_printf("MPU6050: initializing on PB6/PB7...\r\n");
    if (mpu6050_init() != 0)
    {
        /* Keep the console alive even when the sensor is absent. */
        while (1)
        {
            uart_printf("UART OK; MPU6050 unavailable (check PB6/PB7)\r\n");
            delay_ms(1000);
        }
    }
    task_imu_init();
    uart_printf("MPU6050: ready\r\n");
    /* =====================================================
     * Start Application
     * ===================================================== */

    drone_app_run();

    /* Should never reach here */

    while (1)
    {
    }
}
