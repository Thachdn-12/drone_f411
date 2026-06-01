#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

/* =========================================================
 * MPU6050 I2C Address
 * ========================================================= */

#define MPU6050_ADDR          0x68

/* =========================================================
 * Registers
 * ========================================================= */

#define MPU6050_REG_WHO_AM_I  0x75
#define MPU6050_REG_PWR_MGMT1 0x6B

#define MPU6050_REG_ACCEL_XH  0x3B
#define MPU6050_REG_GYRO_XH   0x43

/* =========================================================
 * Raw Data Structure
 * ========================================================= */

typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t gx;
    int16_t gy;
    int16_t gz;

} mpu6050_raw_t;

/* =========================================================
 * APIs
 * ========================================================= */

int mpu6050_init(void);

int mpu6050_read_raw(mpu6050_raw_t *raw);

#endif