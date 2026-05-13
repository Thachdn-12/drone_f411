/**
 * @file    i2c.c
 * @brief   I2C driver implementation for STM32F411 (Baremetal)
 * @author  Toby
 */

#include <stdint.h>
#include "i2c.h"
#include "gpio.h"

/* RCC Base Address */
#define RCC_BASE        0x40023800

/* RCC Registers */
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* RCC Enable Bits */
#define RCC_AHB1ENR_GPIOBEN     (1 << 1)
#define RCC_APB1ENR_I2C1EN     (1 << 21)

/* GPIO Base Addresses */
#define GPIOB_BASE      0x40020400

/* GPIO Registers */
#define GPIO_MODER(n)       *((volatile uint32_t *)((n) + 0x00))
#define GPIO_OTYPER(n)      *((volatile uint32_t *)((n) + 0x04))
#define GPIO_OSPEEDR(n)     *((volatile uint32_t *)((n) + 0x08))
#define GPIO_PUPDR(n)       *((volatile uint32_t *)((n) + 0x0C))
#define GPIO_IDR(n)         *((volatile uint32_t *)((n) + 0x10))
#define GPIO_ODR(n)         *((volatile uint32_t *)((n) + 0x14))
#define GPIO_AFRL(n)        *((volatile uint32_t *)((n) + 0x20))
#define GPIO_AFRH(n)        *((volatile uint32_t *)((n) + 0x24))

/* Timeout definition */
#define I2C_TIMEOUT_VALUE   100000

/**
 * @brief  Initialize I2C1 (PB6-SCL, PB7-SDA) for STM32F411 Blackpill
 * @param  None
 * @retval None
 */
void I2C1_Init(void)
{
    /* Enable GPIOB and I2C1 clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
    
    /* Configure PB6 (SCL) and PB7 (SDA) as Alternate Function */
    /* Set PB6 and PB7 to Alternate Function Mode (MODER = 10) */
    GPIO_MODER(GPIOB_BASE) &= ~(0xF << 12);    /* Clear bits 12-15 */
    GPIO_MODER(GPIOB_BASE) |= (0xA << 12);      /* Set AF mode for PB6, PB7 */
    
    /* Set Output Type to Open-Drain */
    GPIO_OTYPER(GPIOB_BASE) |= (0x3 << 6);      /* Open-drain for PB6, PB7 */
    
    /* Set High Speed */
    GPIO_OSPEEDR(GPIOB_BASE) |= (0xF << 12);    /* High speed for PB6, PB7 */
    
    /* Set Pull-Up */
    GPIO_PUPDR(GPIOB_BASE) &= ~(0xF << 12);
    GPIO_PUPDR(GPIOB_BASE) |= (0x5 << 12);      /* Pull-up for PB6, PB7 */
    
    /* Set Alternate Function 4 (I2C1) */
    GPIO_AFRL(GPIOB_BASE) &= ~(0xFF << 24);     /* Clear AF for PB6, PB7 */
    GPIO_AFRL(GPIOB_BASE) |= (0x44 << 24);      /* AF4 for PB6, PB7 */
    
    /* Reset I2C1 */
    I2C_CR1(I2C1_BASE) |= I2C_CR1_SWRST;
    I2C_CR1(I2C1_BASE) &= ~I2C_CR1_SWRST;
    
    /* Disable I2C1 before configuration */
    I2C_CR1(I2C1_BASE) &= ~I2C_CR1_PE;
    
    /* Configure I2C1 for 100kHz (Standard Mode) */
    /* APB1 Clock = 48MHz (assuming SystemCoreClock = 48MHz) */
    /* Set CR2: FREQ = 48 (APB1 frequency in MHz) */
    I2C_CR2(I2C1_BASE) = 16;
    
    /* Set CCR for 100kHz */
    /* CCR = Fpclk1 / (2 * Fi2c) = 48000000 / (2 * 100000) = 240 */
    I2C_CCR(I2C1_BASE) = 80;
    
    /* Set TRISE for 100kHz */
    /* TRISE = Fpclk1 / 1MHz + 1 = 48 + 1 = 49 */
    I2C_TRISE(I2C1_BASE) = 17;
    
    /* Enable I2C1 */
    I2C_CR1(I2C1_BASE) |= I2C_CR1_PE;
    
    /* Enable Acknowledge */
    I2C_CR1(I2C1_BASE) |= I2C_CR1_ACK;
}

/**
 * @brief  Generate I2C Start Condition
 * @param  i2c_base: I2C peripheral base address
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_Start(uint32_t i2c_base)
{
    volatile uint32_t timeout = I2C_TIMEOUT_VALUE;
    
    /* Wait until bus is not busy */
    while ((I2C_SR2(i2c_base) & I2C_SR2_BUSY) && timeout--);
    if (timeout == 0) return I2C_TIMEOUT;
    
    /* Generate Start Condition */
    I2C_CR1(i2c_base) |= I2C_CR1_START;
    
    /* Wait for Start Bit flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_SB) && timeout--);
    if (timeout == 0) return I2C_TIMEOUT;
    
    return I2C_OK;
}

/**
 * @brief  Generate I2C Stop Condition
 * @param  i2c_base: I2C peripheral base address
 * @retval None
 */
void I2C_Stop(uint32_t i2c_base)
{
    /* Generate Stop Condition */
    I2C_CR1(i2c_base) |= I2C_CR1_STOP;
    
    /* Wait for STOP flag to be cleared */
    volatile uint32_t timeout = I2C_TIMEOUT_VALUE;
    while ((I2C_CR1(i2c_base) & I2C_CR1_STOP) && timeout--);
}

/**
 * @brief  Write one byte to I2C bus
 * @param  i2c_base: I2C peripheral base address
 * @param  data: Data byte to send
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_SendAddress(uint32_t i2c_base,
                             uint8_t addr)
{
    volatile uint32_t timeout = I2C_TIMEOUT_VALUE;

    /* Send address */
    I2C_DR(i2c_base) = addr;

    while (!(I2C_SR1(i2c_base) & I2C_SR1_ADDR))
    {
        /* ACK failure */
        if (I2C_SR1(i2c_base) & I2C_SR1_AF)
        {
            /* clear AF */
            I2C_SR1(i2c_base) &= ~I2C_SR1_AF;

            return I2C_ERROR;
        }

        if (--timeout == 0)
        {
            return I2C_TIMEOUT;
        }
    }

    return I2C_OK;
}
/**
 * @brief  Write one byte to I2C bus
 * @param  i2c_base: I2C peripheral base address
 * @param  data: Data byte to send
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_WriteByte(uint32_t i2c_base, uint8_t data)
{
    volatile uint32_t timeout = I2C_TIMEOUT_VALUE;
    
    /* Wait for TXE flag (Transmit Data Register Empty) */
    while (!(I2C_SR1(i2c_base) & I2C_SR1_TXE) && timeout--);
    if (timeout == 0) return I2C_TIMEOUT;
    
    /* Write data to DR */
    I2C_DR(i2c_base) = data;
    
    /* Wait for BTF flag (Byte Transfer Finished) */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_BTF) && timeout--);
    if (timeout == 0) return I2C_TIMEOUT;
    
    return I2C_OK;
}

/**
 * @brief  Read one byte from I2C bus
 * @param  i2c_base: I2C peripheral base address
 * @param  data: Pointer to store received data
 * @param  ack: Enable ACK (1) or NACK (0)
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_ReadByte(uint32_t i2c_base, uint8_t *data, uint8_t ack)
{
    volatile uint32_t timeout = I2C_TIMEOUT_VALUE;
    
    /* Enable/Disable Acknowledge */
    if (ack) {
        /* ACK = 1 */
        I2C_CR1(i2c_base) |= I2C_CR1_ACK;
        /* clear ADDR */
        (void)I2C_SR1(i2c_base);
        (void)I2C_SR2(i2c_base);
    } else {
        /* ACK = 0 */
        I2C_CR1(i2c_base) &= ~I2C_CR1_ACK;
        /* Stop */
        I2C_CR1(i2c_base) |= I2C_CR1_STOP;
        /* clear ADDR */
        (void)I2C_SR1(i2c_base);
        (void)I2C_SR2(i2c_base);
    }
    
    /* Wait for RXNE flag (Receive Data Register Not Empty) */
    while (!(I2C_SR1(i2c_base) & I2C_SR1_RXNE))
    {
        if (--timeout == 0)
        {
            return I2C_TIMEOUT;
        }
    }
    
    /* Read data from DR */
    *data = (uint8_t)I2C_DR(i2c_base);
    
    return I2C_OK;
}


/**
 * @brief  Write data to a specific register
 * @param  i2c_base: I2C peripheral base address
 * @param  dev_addr: Device address (7-bit)
 * @param  reg: Register address
 * @param  data: Data to write
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_WriteRegister(uint32_t i2c_base, uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    I2C_Status_t status;
    volatile uint32_t timeout;
    
    /* Generate Start */
    status = I2C_Start(i2c_base);
    if (status != I2C_OK) return status;
    
    /* Send device address with write bit */
    I2C_DR(i2c_base) = (dev_addr << 1) | 0x00;
    
    /* Wait for ADDR flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_ADDR) && timeout--);
    if (timeout == 0) {
        I2C_Stop(i2c_base);
        return I2C_TIMEOUT;
    }
    
    /* Check for ACK failure */
    if (I2C_SR1(i2c_base) & I2C_SR1_AF) {
        I2C_SR1(i2c_base) &= ~I2C_SR1_AF;
        I2C_Stop(i2c_base);
        return I2C_NACK;
    }
    
    /* Clear ADDR flag by reading SR2 */
    (void)I2C_SR2(i2c_base);
    
    /* Send register address */
    status = I2C_WriteByte(i2c_base, reg);
    if (status != I2C_OK) {
        I2C_Stop(i2c_base);
        return status;
    }
    
    /* Send data */
    status = I2C_WriteByte(i2c_base, data);
    if (status != I2C_OK) {
        I2C_Stop(i2c_base);
        return status;
    }
    
    /* Generate Stop */
    I2C_Stop(i2c_base);
    
    return I2C_OK;
}

/**
 * @brief  Read data from a specific register
 * @param  i2c_base: I2C peripheral base address
 * @param  dev_addr: Device address (7-bit)
 * @param  reg: Register address
 * @param  data: Pointer to store received data
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_ReadRegister(uint32_t i2c_base, uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    I2C_Status_t status;
    volatile uint32_t timeout;
    
    /* Generate Start */
    status = I2C_Start(i2c_base);
    if (status != I2C_OK) return status;
    
    /* Send device address with write bit */
    I2C_DR(i2c_base) = (dev_addr << 1) | 0x00;
    
    /* Wait for ADDR flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_ADDR) && timeout--);
    if (timeout == 0) {
        I2C_Stop(i2c_base);
        return I2C_TIMEOUT;
    }
    
    /* Clear ADDR flag by reading SR2 */
    (void)I2C_SR2(i2c_base);
    
    /* Send register address */
    status = I2C_WriteByte(i2c_base, reg);
    if (status != I2C_OK) {
        I2C_Stop(i2c_base);
        return status;
    }
    
    /* Generate Repeated Start */
    status = I2C_Start(i2c_base);
    if (status != I2C_OK) return status;
    
    /* Send device address with read bit */
    I2C_DR(i2c_base) = (dev_addr << 1) | 0x01;
    
    /* Wait for ADDR flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_ADDR) && timeout--);
    if (timeout == 0) {
        I2C_Stop(i2c_base);
        return I2C_TIMEOUT;
    }
    
    /* Disable ACK before clearing ADDR */
    I2C_CR1(i2c_base) &= ~I2C_CR1_ACK;
    
    /* Clear ADDR flag by reading SR2 */
    (void)I2C_SR2(i2c_base);
    
    /* Generate Stop after reading */
    I2C_CR1(i2c_base) |= I2C_CR1_STOP;
    
    /* Wait for RXNE flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_RXNE) && timeout--);
    if (timeout == 0) return I2C_TIMEOUT;
    
    /* Read data */
    *data = (uint8_t)I2C_DR(i2c_base);
    
    /* Re-enable ACK for next transfer */
    I2C_CR1(i2c_base) |= I2C_CR1_ACK;
    
    return I2C_OK;
}

/**
 * @brief  Read multiple bytes from sequential registers
 * @param  i2c_base: I2C peripheral base address
 * @param  dev_addr: Device address (7-bit)
 * @param  reg: Starting register address
 * @param  buffer: Pointer to store received data
 * @param  length: Number of bytes to read
 * @retval I2C_Status_t
 */
I2C_Status_t I2C_ReadMultiBytes(uint32_t i2c_base, uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint16_t length)
{
    I2C_Status_t status;
    volatile uint32_t timeout;
    uint16_t i;
    
    /* Generate Start */
    status = I2C_Start(i2c_base);
    if (status != I2C_OK) return status;
    
    /* Send device address with write bit */
    I2C_DR(i2c_base) = (dev_addr << 1) | 0x00;
    
    /* Wait for ADDR flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_ADDR) && timeout--);
    if (timeout == 0) {
        I2C_Stop(i2c_base);
        return I2C_TIMEOUT;
    }
    
    /* Clear ADDR flag by reading SR2 */
    (void)I2C_SR2(i2c_base);
    
    /* Send register address */
    status = I2C_WriteByte(i2c_base, reg);
    if (status != I2C_OK) {
        I2C_Stop(i2c_base);
        return status;
    }
    
    /* Generate Repeated Start */
    status = I2C_Start(i2c_base);
    if (status != I2C_OK) return status;
    
    /* Send device address with read bit */
    I2C_DR(i2c_base) = (dev_addr << 1) | 0x01;
    
    /* Wait for ADDR flag */
    timeout = I2C_TIMEOUT_VALUE;
    while (!(I2C_SR1(i2c_base) & I2C_SR1_ADDR) && timeout--);
    if (timeout == 0) {
        I2C_Stop(i2c_base);
        return I2C_TIMEOUT;
    }
    
    /* Clear ADDR flag by reading SR2 */
    (void)I2C_SR2(i2c_base);
    
    /* Enable ACK */
    I2C_CR1(i2c_base) |= I2C_CR1_ACK;
    
    /* Read data bytes */
    for (i = 0; i < length; i++) {
        /* For last byte, send NACK */
        if (i == (length - 1)) {
            I2C_CR1(i2c_base) &= ~I2C_CR1_ACK;
            I2C_CR1(i2c_base) |= I2C_CR1_STOP;
        }
        
        /* Wait for RXNE flag */
        timeout = I2C_TIMEOUT_VALUE;
        while (!(I2C_SR1(i2c_base) & I2C_SR1_RXNE) && timeout--);
        if (timeout == 0) {
            I2C_Stop(i2c_base);
            return I2C_TIMEOUT;
        }
        
        /* Read data */
        buffer[i] = (uint8_t)I2C_DR(i2c_base);
    }
    
    /* Re-enable ACK for next transfer */
    I2C_CR1(i2c_base) |= I2C_CR1_ACK;
    
    return I2C_OK;
}