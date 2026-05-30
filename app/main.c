#include "systick.h"
#include "uart.h"
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
    systick_init(100000000);

    /* =====================================================
     * Peripheral Drivers
     * ===================================================== */

    i2c_init();
    pwm_init();
    adc_init();
    mpu6050_init();
    task_imu_init();
    /* =====================================================
     * Boot Log
     * ===================================================== */

    uart_printf("\r\n");
    uart_printf("================================\r\n");
    uart_printf(" STM32F411 DRONE FIRMWARE\r\n");
    uart_printf("================================\r\n");

    uart_printf("[OK] SYSTEM INIT\r\n");
    uart_printf("[OK] SYSTICK INIT\r\n");
    uart_printf("[OK] UART INIT\r\n");
    uart_printf("[OK] GPIO INIT\r\n");
    uart_printf("[OK] I2C INIT\r\n");
    uart_printf("[OK] PWM INIT\r\n");
    uart_printf("[OK] ADC INIT\r\n");
    uart_printf("[OK] MPU6050 INIT\r\n");
    uart_printf("[OK] IMU TASK INIT\r\n");
    /* =====================================================
     * Start Application
     * ===================================================== */

    drone_app_run();

    /* Should never reach here */

    while (1)
    {
    }
}