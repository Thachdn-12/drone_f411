/**
 * @file    i2c.h
 * @brief   I2C driver for STM32F411 (Baremetal)
 * @author  Toby
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/* I2C Base Addresses */
#define I2C1_BASE       0x40005400

/* I2C Registers */
#define I2C_CR1(n)      *((volatile uint32_t *)((n) + 0x00))
#define I2C_CR2(n)      *((volatile uint32_t *)((n) + 0x04))
#define I2C_OAR1(n)     *((volatile uint32_t *)((n) + 0x08))
#define I2C_OAR2(n)     *((volatile uint32_t *)((n) + 0x0C))
#define I2C_DR(n)       *((volatile uint32_t *)((n) + 0x10))
#define I2C_SR1(n)      *((volatile uint32_t *)((n) + 0x14))
#define I2C_SR2(n)      *((volatile uint32_t *)((n) + 0x18))
#define I2C_CCR(n)      *((volatile uint32_t *)((n) + 0x1C))
#define I2C_TRISE(n)    *((volatile uint32_t *)((n) + 0x20))
#define I2C_FLTR(n)     *((volatile uint32_t *)((n) + 0x24))

/* I2C CR1 Register Bits */
#define I2C_CR1_PE          (1 << 0)    /* Peripheral Enable */
#define I2C_CR1_START       (1 << 8)    /* Start Generation */
#define I2C_CR1_STOP        (1 << 9)    /* Stop Generation */
#define I2C_CR1_ACK         (1 << 10)   /* Acknowledge Enable */
#define I2C_CR1_POS         (1 << 11)   /* Acknowledge Position */
#define I2C_CR1_SWRST       (1 << 15)   /* Software Reset */

/* I2C CR2 Register Bits */
#define I2C_CR2_ITERREN     (1 << 8)    /* Error Interrupt Enable */
#define I2C_CR2_ITEVTEN     (1 << 9)    /* Event Interrupt Enable */
#define I2C_CR2_ITBUFEN     (1 << 10)   /* Buffer Interrupt Enable */

/* I2C SR1 Register Bits */
#define I2C_SR1_SB          (1 << 0)    /* Start Bit */
#define I2C_SR1_ADDR        (1 << 1)    /* Address Sent */
#define I2C_SR1_BTF         (1 << 2)    /* Byte Transfer Finished */
#define I2C_SR1_ADD10       (1 << 3)    /* 10-bit Header Sent */
#define I2C_SR1_STOPF       (1 << 4)    /* Stop Detection */
#define I2C_SR1_RXNE        (1 << 6)    /* Data Register Not Empty */
#define I2C_SR1_TXE         (1 << 7)    /* Data Register Empty */
#define I2C_SR1_BERR        (1 << 8)    /* Bus Error */
#define I2C_SR1_ARLO        (1 << 9)    /* Arbitration Lost */
#define I2C_SR1_AF          (1 << 10)   /* Acknowledge Failure */
#define I2C_SR1_OVR         (1 << 11)   /* Overrun/Underrun */
#define I2C_SR1_TIMEOUT     (1 << 14)   /* Timeout */

/* I2C SR2 Register Bits */
#define I2C_SR2_MSL         (1 << 0)    /* Master Mode */
#define I2C_SR2_BUSY        (1 << 1)    /* Bus Busy */
#define I2C_SR2_TRA         (1 << 2)    /* Transmitter Mode */
#define I2C_SR2_GENCALL     (1 << 4)    /* General Call Address */
#define I2C_SR2_DUALF       (1 << 7)    /* Dual Flag */

/* I2C CCR Register Bits */
#define I2C_CCR_FS          (1 << 15)   /* Fast Mode */
#define I2C_CCR_DUTY        (1 << 14)   /* Duty Cycle */

/* I2C Status Codes */
typedef enum {
    I2C_OK = 0,
    I2C_ERROR,
    I2C_TIMEOUT,
    I2C_NACK,
    I2C_BUS_ERROR,
    I2C_ARBITRATION_LOST
} I2C_Status_t;

/* Function Prototypes */
void I2C1_Init(void);
I2C_Status_t I2C_Start(uint32_t i2c_base);
void I2C_Stop(uint32_t i2c_base);
I2C_Status_t I2C_SendAddress(uint32_t i2c, uint8_t addr);
I2C_Status_t I2C_WriteByte(uint32_t i2c_base, uint8_t data);
I2C_Status_t I2C_ReadByte(uint32_t i2c_base, uint8_t *data, uint8_t ack);
I2C_Status_t I2C_WriteRegister(uint32_t i2c_base, uint8_t dev_addr, uint8_t reg, uint8_t data);
I2C_Status_t I2C_ReadRegister(uint32_t i2c_base, uint8_t dev_addr, uint8_t reg, uint8_t *data);
I2C_Status_t I2C_ReadMultiBytes(uint32_t i2c_base, uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint16_t length);
#endif