#include "systick.h"
#include "uart.h"
#include "system.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "pwm.h"
#include "adc.h"
#include "mpu6050.h"
#include "task_imu.h"
#include "task_baro.h"
#include "timer.h"

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
    timer_init(1000);
    spi1_init();
    uart_printf("BMP280: initializing SPI1 PA4/PA5/PA6/PA7...\r\n");
    bmp280_status_t baro_status = task_baro_init();
    if (baro_status == BMP280_OK)
        uart_printf("BMP280: ready\r\n");
    else
        uart_printf("BMP280: unavailable (%d), check SPI wiring\r\n", (int)baro_status);
    uart_printf("MPU6050: initializing on PB6/PB7...\r\n");
    if (mpu6050_init() != 0)
    {
        /* Keep the console alive even when the sensor is absent. */
        while (1)
        {
            uart_printf("UART OK; MPU6050 unavailable (check PB6/PB7)\r\n");
            task_baro_run();
            delay_ms(1000);
        }
    }
    task_imu_init();
    uart_printf("MPU6050: ready\r\n");

    uart_printf("[PWM] Check pwm value \n");
    pwm_set_us(PWM_CH1, 1000);
    pwm_set_us(PWM_CH2, 1250);
    pwm_set_us(PWM_CH3, 1500);
    pwm_set_us(PWM_CH4, 2000);
    /* =====================================================
     * Start Application
     * ===================================================== */

    drone_app_run();

    /* Should never reach here */

    while (1)
    {
    }
}
