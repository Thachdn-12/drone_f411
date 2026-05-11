/**
 * @file    mpu6050.h
 * @brief   MPU6050 driver for STM32F411 (Baremetal)
 * @author  Toby
 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

/* MPU6050 I2C Address */
#define MPU6050_ADDR_LOW        0x68    /* AD0 = 0 */
#define MPU6050_ADDR_HIGH       0x69    /* AD0 = 1 */

/* MPU6050 Register Map */
#define MPU6050_SELF_TEST_X     0x0D
#define MPU6050_SELF_TEST_Y     0x0E
#define MPU6050_SELF_TEST_Z     0x0F
#define MPU6050_SELF_TEST_A     0x10
#define MPU6050_SMPLRT_DIV      0x19
#define MPU6050_CONFIG          0x1A
#define MPU6050_GYRO_CONFIG     0x1B
#define MPU6050_ACCEL_CONFIG    0x1C
#define MPU6050_FIFO_EN         0x23
#define MPU6050_I2C_MST_CTRL    0x24
#define MPU6050_I2C_SLV0_ADDR   0x25
#define MPU6050_I2C_SLV0_REG    0x26
#define MPU6050_I2C_SLV0_CTRL   0x27
#define MPU6050_INT_PIN_CFG     0x37
#define MPU6050_INT_ENABLE      0x38
#define MPU6050_INT_STATUS      0x3A
#define MPU6050_ACCEL_XOUT_H    0x3B
#define MPU6050_ACCEL_XOUT_L    0x3C
#define MPU6050_ACCEL_YOUT_H    0x3D
#define MPU6050_ACCEL_YOUT_L    0x3E
#define MPU6050_ACCEL_ZOUT_H    0x3F
#define MPU6050_ACCEL_ZOUT_L    0x40
#define MPU6050_TEMP_OUT_H      0x41
#define MPU6050_TEMP_OUT_L      0x42
#define MPU6050_GYRO_XOUT_H     0x43
#define MPU6050_GYRO_XOUT_L     0x44
#define MPU6050_GYRO_YOUT_H     0x45
#define MPU6050_GYRO_YOUT_L     0x46
#define MPU6050_GYRO_ZOUT_H     0x47
#define MPU6050_GYRO_ZOUT_L     0x48
#define MPU6050_USER_CTRL       0x6A
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_PWR_MGMT_2      0x6C
#define MPU6050_FIFO_COUNT_H    0x72
#define MPU6050_FIFO_COUNT_L    0x73
#define MPU6050_FIFO_R_W        0x74
#define MPU6050_WHO_AM_I        0x75

/* Gyroscope Full Scale Range */
typedef enum {
    MPU6050_GYRO_FS_250  = 0x00,   /* ±250 °/s */
    MPU6050_GYRO_FS_500  = 0x08,   /* ±500 °/s */
    MPU6050_GYRO_FS_1000 = 0x10,   /* ±1000 °/s */
    MPU6050_GYRO_FS_2000 = 0x18    /* ±2000 °/s */
} MPU6050_GyroFS_t;

/* Accelerometer Full Scale Range */
typedef enum {
    MPU6050_ACCEL_FS_2  = 0x00,    /* ±2g */
    MPU6050_ACCEL_FS_4  = 0x08,    /* ±4g */
    MPU6050_ACCEL_FS_8  = 0x10,    /* ±8g */
    MPU6050_ACCEL_FS_16 = 0x18     /* ±16g */
} MPU6050_AccelFS_t;

/* Clock Source */
typedef enum {
    MPU6050_CLOCK_INTERNAL    = 0x00,  /* Internal 8MHz */
    MPU6050_CLOCK_PLL_XGYRO  = 0x01,  /* PLL with X Gyro reference */
    MPU6050_CLOCK_PLL_YGYRO  = 0x02,  /* PLL with Y Gyro reference */
    MPU6050_CLOCK_PLL_ZGYRO  = 0x03,  /* PLL with Z Gyro reference */
    MPU6050_CLOCK_PLL_EXT32K = 0x04,  /* PLL with external 32.768kHz */
    MPU6050_CLOCK_PLL_EXT19M = 0x05,  /* PLL with external 19.2MHz */
    MPU6050_CLOCK_KEEP_RESET = 0x07   /* Stops the clock and keeps timing generator in reset */
} MPU6050_ClockSource_t;

/* Digital Low Pass Filter Bandwidth */
typedef enum {
    MPU6050_DLPF_260HZ = 0x00,  /* Accel: 260Hz, Gyro: 256Hz */
    MPU6050_DLPF_184HZ = 0x01,  /* Accel: 184Hz, Gyro: 188Hz */
    MPU6050_DLPF_94HZ  = 0x02,  /* Accel: 94Hz, Gyro: 98Hz */
    MPU6050_DLPF_44HZ  = 0x03,  /* Accel: 44Hz, Gyro: 42Hz */
    MPU6050_DLPF_21HZ  = 0x04,  /* Accel: 21Hz, Gyro: 20Hz */
    MPU6050_DLPF_10HZ  = 0x05,  /* Accel: 10Hz, Gyro: 10Hz */
    MPU6050_DLPF_5HZ   = 0x06   /* Accel: 5Hz, Gyro: 5Hz */
} MPU6050_DLPF_t;

/* MPU6050 Data Structure */
typedef struct {
    int16_t accel_x;        /* Accelerometer X */
    int16_t accel_y;        /* Accelerometer Y */
    int16_t accel_z;        /* Accelerometer Z */
    int16_t temp;           /* Temperature */
    int16_t gyro_x;         /* Gyroscope X */
    int16_t gyro_y;         /* Gyroscope Y */
    int16_t gyro_z;         /* Gyroscope Z */
} MPU6050_RawData_t;

/* MPU6050 Scaled Data Structure */
typedef struct {
    float accel_x;          /* Accelerometer X (g) */
    float accel_y;          /* Accelerometer Y (g) */
    float accel_z;          /* Accelerometer Z (g) */
    float temp;             /* Temperature (°C) */
    float gyro_x;           /* Gyroscope X (°/s) */
    float gyro_y;           /* Gyroscope Y (°/s) */
    float gyro_z;           /* Gyroscope Z (°/s) */
} MPU6050_Data_t;

/* MPU6050 Configuration Structure */
typedef struct {
    MPU6050_AccelFS_t accel_fs;     /* Accelerometer full scale */
    MPU6050_GyroFS_t gyro_fs;       /* Gyroscope full scale */
    MPU6050_ClockSource_t clk_src;  /* Clock source */
    MPU6050_DLPF_t dlpf;            /* Digital low pass filter */
    uint8_t sample_rate_div;        /* Sample rate divider */
} MPU6050_Config_t;

/* MPU6050 Status */
typedef enum {
    MPU6050_OK = 0,
    MPU6050_ERROR,
    MPU6050_NOT_CONNECTED,
    MPU6050_TIMEOUT
} MPU6050_Status_t;

/* Function Prototypes */
MPU6050_Status_t MPU6050_Init(uint8_t i2c_addr);
MPU6050_Status_t MPU6050_Configure(MPU6050_Config_t *config);
MPU6050_Status_t MPU6050_GetDeviceID(uint8_t *id);
MPU6050_Status_t MPU6050_GetRawData(MPU6050_RawData_t *data);
MPU6050_Status_t MPU6050_GetData(MPU6050_Data_t *data);
MPU6050_Status_t MPU6050_SetSleepMode(uint8_t enable);
MPU6050_Status_t MPU6050_SetAccelFS(MPU6050_AccelFS_t fs);
MPU6050_Status_t MPU6050_SetGyroFS(MPU6050_GyroFS_t fs);
MPU6050_Status_t MPU6050_SetClockSource(MPU6050_ClockSource_t clk);
MPU6050_Status_t MPU6050_SetDLPF(MPU6050_DLPF_t dlpf);
MPU6050_Status_t MPU6050_SetSampleRateDiv(uint8_t div);

/* Helper Functions */
float MPU6050_GetAccelSensitivity(MPU6050_AccelFS_t fs);
float MPU6050_GetGyroSensitivity(MPU6050_GyroFS_t fs);

#endif /* MPU6050_H */