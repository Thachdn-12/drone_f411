/* =========================================================
 * File: drivers/i2c/i2c.c
 * STM32F411 I2C1 Driver
 * PB8 -> SCL
 * PB9 -> SDA
 * ========================================================= */

#include "i2c.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"
/* =========================================================
 * Base Addresses
 * ========================================================= */

#define RCC_BASE            0x40023800UL
#define GPIOB_BASE          0x40020400UL
#define I2C1_BASE           0x40005400UL

/* =========================================================
 * RCC
 * ========================================================= */

#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* =========================================================
 * GPIOB
 * ========================================================= */

#define GPIOB_MODER         (*(volatile uint32_t*)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER        (*(volatile uint32_t*)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR       (*(volatile uint32_t*)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR         (*(volatile uint32_t*)(GPIOB_BASE + 0x0C))
#define GPIOB_AFRH          (*(volatile uint32_t*)(GPIOB_BASE + 0x24))

/* =========================================================
 * I2C1 Registers
 * ========================================================= */

#define I2C1_CR1            (*(volatile uint32_t*)(I2C1_BASE + 0x00))
#define I2C1_CR2            (*(volatile uint32_t*)(I2C1_BASE + 0x04))
#define I2C1_OAR1           (*(volatile uint32_t*)(I2C1_BASE + 0x08))
#define I2C1_SR1            (*(volatile uint32_t*)(I2C1_BASE + 0x14))
#define I2C1_SR2            (*(volatile uint32_t*)(I2C1_BASE + 0x18))
#define I2C1_DR             (*(volatile uint32_t*)(I2C1_BASE + 0x10))
#define I2C1_CCR            (*(volatile uint32_t*)(I2C1_BASE + 0x1C))
#define I2C1_TRISE          (*(volatile uint32_t*)(I2C1_BASE + 0x20))

/* =========================================================
 * Bit Definitions
 * ========================================================= */

#define I2C_CR1_PE          (1 << 0)
#define I2C_CR1_START       (1 << 8)
#define I2C_CR1_STOP        (1 << 9)
#define I2C_CR1_ACK         (1 << 10)

#define I2C_SR1_SB          (1 << 0)
#define I2C_SR1_ADDR        (1 << 1)
#define I2C_SR1_BTF         (1 << 2)
#define I2C_SR1_RXNE        (1 << 6)
#define I2C_SR1_TXE         (1 << 7)

#define I2C_SR2_BUSY        (1 << 1)

/* =========================================================
 * Local Helpers
 * ========================================================= */

static void i2c_start(void)
{
    I2C1_CR1 |= I2C_CR1_START;

    while (!(I2C1_SR1 & I2C_SR1_SB))
    {
    }
}

static void i2c_stop(void)
{
    I2C1_CR1 |= I2C_CR1_STOP;
}

static void i2c_send_addr(uint8_t addr)
{
    I2C1_DR = addr;

    while (!(I2C1_SR1 & I2C_SR1_ADDR))
    {
    }

    /*
     * Clear ADDR flag
     */

    (void)I2C1_SR1;
    (void)I2C1_SR2;
}

static void i2c_write_byte(uint8_t data)
{
    while (!(I2C1_SR1 & I2C_SR1_TXE))
    {
    }

    I2C1_DR = data;

    while (!(I2C1_SR1 & I2C_SR1_BTF))
    {
    }
}

static uint8_t i2c_read_byte_ack(void)
{
    I2C1_CR1 |= I2C_CR1_ACK;

    while (!(I2C1_SR1 & I2C_SR1_RXNE))
    {
    }

    return (uint8_t)I2C1_DR;
}

static uint8_t i2c_read_byte_nack(void)
{
    I2C1_CR1 &= ~I2C_CR1_ACK;

    while (!(I2C1_SR1 & I2C_SR1_RXNE))
    {
    }

    return (uint8_t)I2C1_DR;
}

/* =========================================================
 * Init
 * ========================================================= */

void i2c_init(void)
{
    /*
     * Enable clocks
     */

    RCC_AHB1ENR |= (1 << 1);   /* GPIOB */
    RCC_APB1ENR |= (1 << 21);  /* I2C1 */

    /* =====================================================
     * PB8 -> I2C1_SCL
     * PB9 -> I2C1_SDA
     * AF4
     * ===================================================== */

    /* Alternate function */
    gpio_mode(GPIOB, 8, GPIO_AF);
    gpio_mode(GPIOB, 9, GPIO_AF);

    /* Open drain */
    gpio_output_type(GPIOB, 8, GPIO_OPEN_DRAIN);
    gpio_output_type(GPIOB, 9, GPIO_OPEN_DRAIN);

    /* High speed */
    gpio_speed(GPIOB, 8, GPIO_HIGH_SPEED);
    gpio_speed(GPIOB, 9, GPIO_HIGH_SPEED);

    /* Pull-up */
    gpio_pull(GPIOB, 8, GPIO_PULL_UP);
    gpio_pull(GPIOB, 9, GPIO_PULL_UP);

    /* AF4 */
    gpio_af(GPIOB, 8, 0x4);
    gpio_af(GPIOB, 9, 0x4);

    /* =====================================================
     * Reset I2C
     * ===================================================== */

    I2C1_CR1 = 0;

    /*
     * APB1 clock = 50MHz
     */

    I2C1_CR2 = 50;

    /*
     * Standard mode 100kHz
     */

    I2C1_CCR = 250;

    /*
     * Maximum rise time
     */

    I2C1_TRISE = 51;

    /* Enable ACK */
    I2C1_CR1 |= I2C_CR1_ACK;

    /* Enable peripheral */
    I2C1_CR1 |= I2C_CR1_PE;
}

/* =========================================================
 * Write Register
 * ========================================================= */

int i2c_write_reg(uint8_t dev_addr,
                  uint8_t reg,
                  uint8_t data)
{
    while (I2C1_SR2 & I2C_SR2_BUSY)
    {
    }

    i2c_start();

    /*
     * Write mode
     */

    i2c_send_addr((dev_addr << 1) | 0);

    i2c_write_byte(reg);

    i2c_write_byte(data);

    i2c_stop();

    return 0;
}

/* =========================================================
 * Read Register
 * ========================================================= */

int i2c_read_reg(uint8_t dev_addr,
                 uint8_t reg,
                 uint8_t *data)
{
    if (data == 0)
    {
        return -1;
    }

    while (I2C1_SR2 & I2C_SR2_BUSY)
    {
    }

    /*
     * Write register address
     */

    i2c_start();

    i2c_send_addr((dev_addr << 1) | 0);

    i2c_write_byte(reg);

    /*
     * Repeated start
     */

    i2c_start();

    /*
     * Read mode
     */

    i2c_send_addr((dev_addr << 1) | 1);

    *data = i2c_read_byte_nack();

    i2c_stop();

    return 0;
}

/* =========================================================
 * Read Multiple Bytes
 * ========================================================= */

int i2c_read_bytes(uint8_t dev_addr,
                   uint8_t reg,
                   uint8_t *buf,
                   uint32_t len)
{
    if ((buf == 0) || (len == 0))
    {
        return -1;
    }

    while (I2C1_SR2 & I2C_SR2_BUSY)
    {
    }

    /*
     * Send register address
     */

    i2c_start();

    i2c_send_addr((dev_addr << 1) | 0);

    i2c_write_byte(reg);

    /*
     * Repeated start
     */

    i2c_start();

    i2c_send_addr((dev_addr << 1) | 1);

    for (uint32_t i = 0; i < len; i++)
    {
        if (i == (len - 1))
        {
            buf[i] = i2c_read_byte_nack();
        }
        else
        {
            buf[i] = i2c_read_byte_ack();
        }
    }

    i2c_stop();

    return 0;
}