#include "uart.h"
#include "gpio.h"
#include "systick.h"
#include "adc.h"
#include "pwm.h"
#include "mpu6050.h"

#define GPIO_PIN_13 13
/* =========================================================
 * Local Test Functions
 * ========================================================= */

static void test_uart(void);

static void test_gpio(void);

static void test_pwm(void);

static void test_adc(void);

static void test_mpu6050(void);

/* =========================================================
 * App Run
 * ========================================================= */

void drone_app_run(void)
{
    uart_printf("\r\n");
    uart_printf("==== MODULE TEST START ====\r\n");

    test_uart();

    test_gpio();

    test_pwm();

    test_adc();

    test_mpu6050();

    uart_printf("==== ALL TEST DONE ====\r\n");

    while (1)
    {
        gpio_toggle(GPIOC, GPIO_PIN_13);

        delay_ms(500);
    }
}

/* =========================================================
 * UART Test
 * ========================================================= */

static void test_uart(void)
{
    uart_printf("[TEST] UART\r\n");

    uart_printf("UART PRINTF OK\r\n");

    uart_printf("[OK] UART\r\n");
}

/* =========================================================
 * GPIO Test
 * ========================================================= */

static void test_gpio(void)
{
    uart_printf("[TEST] GPIO\r\n");

    for (int i = 0; i < 10; i++)
    {
        gpio_toggle(GPIOC, GPIO_PIN_13);

        delay_ms(100);
    }

    uart_printf("[OK] GPIO\r\n");
}

/* =========================================================
 * PWM Test
 * WARNING:
 * REMOVE PROPELLERS FIRST
 * ========================================================= */

static void test_pwm(void)
{
    uart_printf("[TEST] PWM\r\n");

    uart_printf("PWM = 1000us\r\n");

    pwm_set_us(PWM_CH1, 1000);

    delay_ms(2000);

    uart_printf("PWM = 1500us\r\n");

    pwm_set_us(PWM_CH1, 1500);

    delay_ms(2000);

    uart_printf("PWM = 1000us\r\n");

    pwm_set_us(PWM_CH1, 1000);

    delay_ms(1000);

    uart_printf("[OK] PWM\r\n");
}

/* =========================================================
 * ADC Test
 * ========================================================= */

static void test_adc(void)
{
    uint16_t adc;

    uart_printf("[TEST] ADC\r\n");

    for (int i = 0; i < 10; i++)
    {
        adc = adc_read(ADC_CHANNEL_0);

        uart_printf("ADC = %u\r\n", adc);

        delay_ms(300);
    }

    uart_printf("[OK] ADC\r\n");
}

/* =========================================================
 * MPU6050 Test
 * ========================================================= */

static void test_mpu6050(void)
{
    mpu6050_raw_t imu;

    uart_printf("[TEST] MPU6050\r\n");

    if (mpu6050_init() != 0)
    {
        uart_printf("[FAIL] MPU6050 INIT\r\n");
        return;
    }

    uart_printf("[OK] MPU6050 INIT\r\n");

    for (int i = 0; i < 20; i++)
    {
        if (mpu6050_read_raw(&imu) == 0)
        {
            uart_printf(
                "AX=%d AY=%d AZ=%d "
                "GX=%d GY=%d GZ=%d\r\n",
                imu.ax,
                imu.ay,
                imu.az,
                imu.gx,
                imu.gy,
                imu.gz
            );
        }
        else
        {
            uart_printf("MPU READ FAIL\r\n");
        }

        delay_ms(200);
    }

    uart_printf("[OK] MPU6050\r\n");
}