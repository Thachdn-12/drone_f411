#include "mpu6050.h"
#include "i2c.h"
#include "uart.h"
#include "systick.h"

/* =========================================================
 * Local Helpers
 * ========================================================= */

static int mpu6050_write_reg(uint8_t reg, uint8_t value)
{
    return i2c_write_reg(MPU6050_ADDR, reg, value);
}

static int mpu6050_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_read_reg(MPU6050_ADDR, reg, value);
}

static int mpu6050_read_bytes(uint8_t reg, uint8_t *buf, uint32_t len)
{
    return i2c_read_bytes(MPU6050_ADDR, reg, buf, len);
}

/* =========================================================
 * Init
 * ========================================================= */

int mpu6050_init(void)
{
    uint8_t id = 0;

    delay_ms(100);

    /* Wake up MPU6050 */
    if (mpu6050_write_reg(MPU6050_REG_PWR_MGMT1, 0x00) != 0)
    {
        uart_printf("MPU: wakeup failed\r\n");
        return -1;
    }

    delay_ms(10);

    /* Read WHO_AM_I */
    if (mpu6050_read_reg(MPU6050_REG_WHO_AM_I, &id) != 0)
    {
        uart_printf("MPU: WHO_AM_I read failed\r\n");
        return -1;
    }

    uart_printf("MPU WHO_AM_I = 0x%02X\r\n", id);

    if (id != 0x68)
    {
        uart_printf("MPU: invalid device\r\n");
        return -1;
    }

    uart_printf("MPU6050 init OK\r\n");

    return 0;
}

/* =========================================================
 * Read Raw Sensor Data
 * ========================================================= */

int mpu6050_read_raw(mpu6050_raw_t *raw)
{
    uint8_t buf[14];

    if (raw == 0)
    {
        return -1;
    }

    /*
     * Read:
     * ACCEL_X/Y/Z
     * TEMP
     * GYRO_X/Y/Z
     */

    if (mpu6050_read_bytes(MPU6050_REG_ACCEL_XH, buf, 14) != 0)
    {
        return -1;
    }

    raw->ax = (int16_t)((buf[0] << 8) | buf[1]);
    raw->ay = (int16_t)((buf[2] << 8) | buf[3]);
    raw->az = (int16_t)((buf[4] << 8) | buf[5]);

    raw->gx = (int16_t)((buf[8] << 8) | buf[9]);
    raw->gy = (int16_t)((buf[10] << 8) | buf[11]);
    raw->gz = (int16_t)((buf[12] << 8) | buf[13]);

    return 0;
}