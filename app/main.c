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
#include "drone_config.h"

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
    
    if (uart_init() != UART_OK)
    {
        while (1) { }
    }
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
    if (timer_init(DRONE_TIMER_HZ) != 0)
    {
        uart_printf("TIM2: initialization failed\r\n");
        while (1) { }
    }
    spi1_init();
    task_baro_set_logging(!DRONE_GCS_ENABLED);
    uart_printf("BMP280: initializing SPI1 PA4/PA5/PA6/PA7...\r\n");
    bmp280_status_t baro_status = task_baro_init();
    if (baro_status == BMP280_OK)
        uart_printf("BMP280: ready\r\n");
    else
        uart_printf("BMP280: unavailable (%d), check SPI wiring\r\n", (int)baro_status);
    uart_printf("MPU6050: initializing on PB6/PB7...\r\n");
    task_imu_init();
    if (mpu6050_init() != 0)
    {
        task_imu_set_available(false);
        uart_printf("MPU6050: unavailable; GCS diagnostics remain active\r\n");
    }
    else uart_printf("MPU6050: ready\r\n");
    /* =====================================================
     * Start Application
     * ===================================================== */

    drone_app_run();

    /* Should never reach here */

    while (1)
    {
    }
}
