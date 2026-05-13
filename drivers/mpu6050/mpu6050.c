/**
 * @file    mpu6050.c
 * @brief   MPU6050 driver implementation for STM32F411 (Baremetal)
 * @author  Toby
 */

#include "mpu6050.h"
#include "i2c.h"

/* Private variables */
static uint8_t mpu6050_addr = MPU6050_ADDR_LOW;
static MPU6050_AccelFS_t current_accel_fs = MPU6050_ACCEL_FS_2;
static MPU6050_GyroFS_t current_gyro_fs = MPU6050_GYRO_FS_250;

uint8_t MPU6050_ReadReg(uint8_t reg)
{
    uint8_t data;

    /* ================= START ================= */
    I2C_Start(I2C1_BASE);

    /* ================= SEND ADDRESS WRITE ================= */
    if(I2C_SendAddress(I2C1_BASE, (0x68 << 1) | 0) != I2C_OK)
        return 0xFF;

    /* ================= SEND REGISTER ================= */
    I2C_WriteData(I2C1_BASE, reg);

    while(!(I2C_SR1(I2C1_BASE) & (1 << 7))); // TXE

    /* ================= RESTART ================= */
    I2C_Start(I2C1_BASE);

    /* ================= SEND ADDRESS READ ================= */
    if(I2C_SendAddress(I2C1_BASE, (0x68 << 1) | 1) != I2C_OK)
        return 0xFF;

    /* ================= DISABLE ACK ================= */
    I2C_CR1(I2C1_BASE) &= ~(1 << 10);

    /* ================= CLEAR ADDR ================= */
    volatile uint32_t temp;
    temp = I2C_SR1(I2C1_BASE);
    temp = I2C_SR2(I2C1_BASE);

    /* ================= STOP ================= */
    I2C_Stop(I2C1_BASE);

    /* ================= WAIT RXNE ================= */
    while(!(I2C_SR1(I2C1_BASE) & (1 << 6)));

    /* ================= READ DATA ================= */
    data = I2C_DR(I2C1_BASE);

    return data;
}

/**
 * @brief  Get accelerometer sensitivity based on full scale range
 * @param  fs: Full scale range
 * @retval Sensitivity in LSB/g
 */
float MPU6050_GetAccelSensitivity(MPU6050_AccelFS_t fs)
{
    switch (fs) {
        case MPU6050_ACCEL_FS_2:  return 16384.0f;
        case MPU6050_ACCEL_FS_4:  return 8192.0f;
        case MPU6050_ACCEL_FS_8:  return 4096.0f;
        case MPU6050_ACCEL_FS_16: return 2048.0f;
        default: return 16384.0f;
    }
}

/**
 * @brief  Get gyroscope sensitivity based on full scale range
 * @param  fs: Full scale range
 * @retval Sensitivity in LSB/(°/s)
 */
float MPU6050_GetGyroSensitivity(MPU6050_GyroFS_t fs)
{
    switch (fs) {
        case MPU6050_GYRO_FS_250:  return 131.0f;
        case MPU6050_GYRO_FS_500:  return 65.5f;
        case MPU6050_GYRO_FS_1000: return 32.8f;
        case MPU6050_GYRO_FS_2000: return 16.4f;
        default: return 131.0f;
    }
}

/**
 * @brief  Initialize MPU6050
 * @param  i2c_addr: I2C address (MPU6050_ADDR_LOW or MPU6050_ADDR_HIGH)
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_Init(uint8_t i2c_addr)
{
    uint8_t id;
    
    mpu6050_addr = i2c_addr;
    
    /* Check device ID */
    if (MPU6050_GetDeviceID(&id) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    /* Expected ID for MPU6050 is 0x68 */
    if (id != 0x68) {
        return MPU6050_NOT_CONNECTED;
    }
    
    /* Wake up MPU6050 (exit sleep mode) */
    if (MPU6050_SetSleepMode(0) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    /* Set default clock source (PLL with X Gyro reference) */
    if (MPU6050_SetClockSource(MPU6050_CLOCK_PLL_XGYRO) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    /* Set default full scale ranges */
    if (MPU6050_SetAccelFS(MPU6050_ACCEL_FS_2) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    if (MPU6050_SetGyroFS(MPU6050_GYRO_FS_250) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    /* Set default DLPF */
    if (MPU6050_SetDLPF(MPU6050_DLPF_94HZ) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}

/**
 * @brief  Configure MPU6050 with custom settings
 * @param  config: Pointer to configuration structure
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_Configure(MPU6050_Config_t *config)
{
    if (config == 0) {
        return MPU6050_ERROR;
    }
    
    if (MPU6050_SetAccelFS(config->accel_fs) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    if (MPU6050_SetGyroFS(config->gyro_fs) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    if (MPU6050_SetClockSource(config->clk_src) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    if (MPU6050_SetDLPF(config->dlpf) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    if (MPU6050_SetSampleRateDiv(config->sample_rate_div) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}

/**
 * @brief  Get device ID (WHO_AM_I register)
 * @param  id: Pointer to store device ID
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_GetDeviceID(uint8_t *id)
{
    I2C_Status_t status;
    
    status = I2C_ReadRegister(I2C1_BASE, mpu6050_addr, MPU6050_WHO_AM_I, id);
    
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}

/**
 * @brief  Get raw sensor data
 * @param  data: Pointer to raw data structure
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_GetRawData(MPU6050_RawData_t *data)
{
    I2C_Status_t status;
    uint8_t buffer[14];
    
    /* Read 14 bytes starting from ACCEL_XOUT_H */
    status = I2C_ReadMultiBytes(I2C1_BASE, mpu6050_addr, MPU6050_ACCEL_XOUT_H, buffer, 14);
    
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    /* Combine high and low bytes */
    data->accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    data->accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    data->accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);
    data->temp    = (int16_t)((buffer[6] << 8) | buffer[7]);
    data->gyro_x  = (int16_t)((buffer[8] << 8) | buffer[9]);
    data->gyro_y  = (int16_t)((buffer[10] << 8) | buffer[11]);
    data->gyro_z  = (int16_t)((buffer[12] << 8) | buffer[13]);
    
    return MPU6050_OK;
}

/**
 * @brief  Get scaled sensor data
 * @param  data: Pointer to scaled data structure
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_GetData(MPU6050_Data_t *data)
{
    MPU6050_RawData_t raw;
    float accel_sens, gyro_sens;
    
    if (MPU6050_GetRawData(&raw) != MPU6050_OK) {
        return MPU6050_ERROR;
    }
    
    /* Get sensitivity values */
    accel_sens = MPU6050_GetAccelSensitivity(current_accel_fs);
    gyro_sens = MPU6050_GetGyroSensitivity(current_gyro_fs);
    
    /* Convert to physical units */
    data->accel_x = (float)raw.accel_x / accel_sens;
    data->accel_y = (float)raw.accel_y / accel_sens;
    data->accel_z = (float)raw.accel_z / accel_sens;
    
    /* Temperature formula: Temp = (raw / 340) + 36.53 */
    data->temp = ((float)raw.temp / 340.0f) + 36.53f;
    
    data->gyro_x = (float)raw.gyro_x / gyro_sens;
    data->gyro_y = (float)raw.gyro_y / gyro_sens;
    data->gyro_z = (float)raw.gyro_z / gyro_sens;
    
    return MPU6050_OK;
}

/**
 * @brief  Enable/disable sleep mode
 * @param  enable: 1 to enable sleep, 0 to wake up
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_SetSleepMode(uint8_t enable)
{
    I2C_Status_t status;
    uint8_t reg;
    
    /* Read current PWR_MGMT_1 value */
    status = I2C_ReadRegister(I2C1_BASE, mpu6050_addr, MPU6050_PWR_MGMT_1, &reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    /* Set/clear SLEEP bit */
    if (enable) {
        reg |= 0x40;
    } else {
        reg &= ~0x40;
    }
    
    /* Write back */
    status = I2C_WriteRegister(I2C1_BASE, mpu6050_addr, MPU6050_PWR_MGMT_1, reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}

/**
 * @brief  Set accelerometer full scale range
 * @param  fs: Full scale range
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_SetAccelFS(MPU6050_AccelFS_t fs)
{
    I2C_Status_t status;
    uint8_t reg;
    
    /* Read current ACCEL_CONFIG value */
    status = I2C_ReadRegister(I2C1_BASE, mpu6050_addr, MPU6050_ACCEL_CONFIG, &reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    /* Clear FS bits and set new value */
    reg &= ~0x18;
    reg |= fs;
    
    /* Write back */
    status = I2C_WriteRegister(I2C1_BASE, mpu6050_addr, MPU6050_ACCEL_CONFIG, reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    current_accel_fs = fs;
    
    return MPU6050_OK;
}

/**
 * @brief  Set gyroscope full scale range
 * @param  fs: Full scale range
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_SetGyroFS(MPU6050_GyroFS_t fs)
{
    I2C_Status_t status;
    uint8_t reg;
    
    /* Read current GYRO_CONFIG value */
    status = I2C_ReadRegister(I2C1_BASE, mpu6050_addr, MPU6050_GYRO_CONFIG, &reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    /* Clear FS bits and set new value */
    reg &= ~0x18;
    reg |= fs;
    
    /* Write back */
    status = I2C_WriteRegister(I2C1_BASE, mpu6050_addr, MPU6050_GYRO_CONFIG, reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    current_gyro_fs = fs;
    
    return MPU6050_OK;
}

/**
 * @brief  Set clock source
 * @param  clk: Clock source
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_SetClockSource(MPU6050_ClockSource_t clk)
{
    I2C_Status_t status;
    uint8_t reg;
    
    /* Read current PWR_MGMT_1 value */
    status = I2C_ReadRegister(I2C1_BASE, mpu6050_addr, MPU6050_PWR_MGMT_1, &reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    /* Clear CLKSEL bits and set new value */
    reg &= ~0x07;
    reg |= clk;
    
    /* Write back */
    status = I2C_WriteRegister(I2C1_BASE, mpu6050_addr, MPU6050_PWR_MGMT_1, reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}

/**
 * @brief  Set Digital Low Pass Filter
 * @param  dlpf: DLPF bandwidth
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_SetDLPF(MPU6050_DLPF_t dlpf)
{
    I2C_Status_t status;
    uint8_t reg;
    
    /* Read current CONFIG value */
    status = I2C_ReadRegister(I2C1_BASE, mpu6050_addr, MPU6050_CONFIG, &reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    /* Clear DLPF bits and set new value */
    reg &= ~0x07;
    reg |= dlpf;
    
    /* Write back */
    status = I2C_WriteRegister(I2C1_BASE, mpu6050_addr, MPU6050_CONFIG, reg);
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}

/**
 * @brief  Set sample rate divider
 * @param  div: Sample rate divider value
 * @retval MPU6050_Status_t
 */
MPU6050_Status_t MPU6050_SetSampleRateDiv(uint8_t div)
{
    I2C_Status_t status;
    
    status = I2C_WriteRegister(I2C1_BASE, mpu6050_addr, MPU6050_SMPLRT_DIV, div);
    
    if (status != I2C_OK) {
        return MPU6050_ERROR;
    }
    
    return MPU6050_OK;
}