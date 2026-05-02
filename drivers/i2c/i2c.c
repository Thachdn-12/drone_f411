#include <stdint.h>

#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x40))

#define GPIOB_BASE      0x40020400
#define GPIOB_MODER     (*(volatile uint32_t*)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER    (*(volatile uint32_t*)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR   (*(volatile uint32_t*)(GPIOB_BASE + 0x08))
#define GPIOB_AFRL      (*(volatile uint32_t*)(GPIOB_BASE + 0x20))
#define GPIOB_PUPDR     (*(volatile uint32_t*)(GPIOB_BASE + 0x0C))

#define I2C1_BASE       0x40005400
#define I2C1_CR1        (*(volatile uint32_t*)(I2C1_BASE + 0x00))
#define I2C1_CR2        (*(volatile uint32_t*)(I2C1_BASE + 0x04))
#define I2C1_CCR        (*(volatile uint32_t*)(I2C1_BASE + 0x1C))
#define I2C1_TRISE      (*(volatile uint32_t*)(I2C1_BASE + 0x20))

#define I2C1_SR1        (*(volatile uint32_t*)(I2C1_BASE + 0x14))
#define I2C1_SR2        (*(volatile uint32_t*)(I2C1_BASE + 0x18))
#define I2C1_DR         (*(volatile uint32_t*)(I2C1_BASE + 0x10))


/* INIT */
void i2c_init(void)
{
    /* 1. Enable clock */
    RCC_AHB1ENR |= (1 << 1);  // GPIOB
    RCC_APB1ENR |= (1 << 21); // I2C1

    /* 2. GPIO config PB6 (SCL), PB7 (SDA) */
    // MODER = AF
    GPIOB_MODER &= ~(0xF << (6*2));
    GPIOB_MODER |=  (0xA << (6*2));

    // Open-drain
    GPIOB_OTYPER |= (1 << 6) | (1 << 7);

    // High speed
    GPIOB_OSPEEDR |= (0xF << (6*2));

    // Pull-up 
    GPIOB_PUPDR &= ~(0xF << (6*2));
    GPIOB_PUPDR |=  (0x5 << (6*2));   // pull-up PB6, PB7

    // AF4 (I2C)
    GPIOB_AFRL &= ~((0xF << (6*4)) | (0xF << (7*4)));
    GPIOB_AFRL |=  ((4 << (6*4)) | (4 << (7*4)));

    /* 3. Reset I2C peripheral */
    I2C1_CR1 |= (1 << 15);
    I2C1_CR1 &= ~(1 << 15);

    /* 4. Disable before config */
    I2C1_CR1 &= ~(1 << 0);

    /* 5. Configure timing */
    I2C1_CR2 = 16;     // APB1 = 16 MHz
    I2C1_CCR = 80;     // 100 kHz
    I2C1_TRISE = 17;   // (1000ns / Tclk) + 1

    /* 6. Enable ACK */
    I2C1_CR1 |= (1 << 10); 

    /* 7. Enable I2C */
    I2C1_CR1 |= (1 << 0); 

}


/* RECEIVER */
void i2c_read(uint8_t addr, uint8_t reg, uint8_t *data)
{
    // START
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0))); // SB

    // send address (write)
    I2C1_DR = addr << 1;
    while (!(I2C1_SR1 & (1 << 1))); // ADDR
    (void)I2C1_SR2;

    // send register
    while (!(I2C1_SR1 & (1 << 7))); // TXE
    I2C1_DR = reg;

    while (!(I2C1_SR1 & (1 << 2))); // BTF

    // RESTART
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0))); // SB

    // send address (read)
    I2C1_DR = (addr << 1) | 1;
    while (!(I2C1_SR1 & (1 << 1))); // ADDR

    // disable ACK (1 byte read)
    I2C1_CR1 &= ~(1 << 10);

    (void)I2C1_SR2;

    // STOP
    I2C1_CR1 |= (1 << 9);

    while (!(I2C1_SR1 & (1 << 6))); // RXNE

    *data = I2C1_DR;
}

void i2c_write(uint8_t addr, uint8_t reg, uint8_t data)
{
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    I2C1_DR = addr << 1;
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR2;

    while (!(I2C1_SR1 & (1 << 7)));
    I2C1_DR = reg;

    while (!(I2C1_SR1 & (1 << 7)));
    I2C1_DR = data;

    while (!(I2C1_SR1 & (1 << 2)));

    I2C1_CR1 |= (1 << 9);
}