#include "spi.h"
#include "gpio.h"
#include "systick.h"

#define RCC_APB2RSTR (*(volatile uint32_t *)(RCC_BASE + 0x24))
#define RCC_APB2ENR  (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define SPI1_CR1     (*(volatile uint32_t *)0x40013000UL)
#define SPI1_CR2     (*(volatile uint32_t *)0x40013004UL)
#define SPI1_SR      (*(volatile uint32_t *)0x40013008UL)
/* Byte access keeps each transfer exactly one 8-bit frame. */
#define SPI1_DR      (*(volatile uint8_t *)0x4001300CUL)
#define SPI1_I2SCFGR (*(volatile uint32_t *)0x4001301CUL)

#define SPI_CLOCK    (1U << 12)
#define SPI_SPE      (1U << 6)
#define SPI_RXNE     (1U << 0)
#define SPI_TXE      (1U << 1)
#define SPI_BSY      (1U << 7)
#define SPI_ERRORS   ((1U << 4) | (1U << 5) | (1U << 6) | (1U << 8))
#define SPI_TIMEOUT_MS 20U
#define SPI_POLL_LIMIT 100000U

static void spi1_reset(void)
{
    RCC_APB2RSTR |= SPI_CLOCK;
    (void)RCC_APB2RSTR;
    RCC_APB2RSTR &= ~SPI_CLOCK;
    SPI1_I2SCFGR = 0; /* SPI, not I2S. */
    SPI1_CR2 = 0;
    /* MSTR, BR=011 (/16), SSM, SSI; CPOL=CPHA=0, DFF=0. */
    SPI1_CR1 = (1U << 2) | (3U << 3) | (1U << 9) | (1U << 8);
    SPI1_CR1 |= SPI_SPE;
}

static int spi1_wait(uint32_t mask, uint32_t expected)
{
    uint32_t start = systick_get_tick();
    for (uint32_t poll = 0; poll < SPI_POLL_LIMIT; ++poll)
    {
        uint32_t status = SPI1_SR;
        if (status & SPI_ERRORS)
            return -1;
        if ((status & mask) == expected)
            return 0;
        if ((uint32_t)(systick_get_tick() - start) >= SPI_TIMEOUT_MS)
            return -1;
    }
    return -1;
}

void spi1_init(void)
{
    gpio_clock_enable(GPIOA);
    RCC_APB2ENR |= SPI_CLOCK;
    (void)RCC_APB2ENR;
    for (uint8_t pin = 5; pin <= 7; ++pin)
    {
        gpio_af(GPIOA, pin, 5);
        gpio_output_type(GPIOA, pin, GPIO_PUSH_PULL);
        gpio_pull(GPIOA, pin, GPIO_NO_PULL);
        gpio_speed(GPIOA, pin, GPIO_FAST_SPEED);
        gpio_mode(GPIOA, pin, GPIO_AF);
    }
    spi1_reset();
}

int spi1_transfer(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    if ((len == 0U) || ((tx == 0) && (rx == 0)) || !(SPI1_CR1 & SPI_SPE))
        return -1;

    for (uint32_t i = 0; i < len; ++i)
    {
        if (spi1_wait(SPI_TXE, SPI_TXE) != 0)
            goto fail;
        SPI1_DR = tx ? tx[i] : 0xFFU;
        if (spi1_wait(SPI_RXNE, SPI_RXNE) != 0)
            goto fail;
        uint8_t value = SPI1_DR; /* Always drain RX to prevent overrun. */
        if (rx)
            rx[i] = value;
    }
    /* RXNE alone does not guarantee that the final clock edge is complete. */
    if ((spi1_wait(SPI_TXE, SPI_TXE) != 0) || (spi1_wait(SPI_BSY, 0) != 0))
        goto fail;
    return 0;

fail:
    /* Reset clears MODF/OVR and any incomplete frame before a later retry. */
    spi1_reset();
    return -1;
}
