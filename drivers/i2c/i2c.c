#include "i2c.h"
#include "gpio.h"
#include "systick.h"
#include "system.h"

#define I2C1_BASE 0x40005400UL
#define RCC_APB1RSTR (*(volatile uint32_t *)(RCC_BASE + 0x20U))
#define RCC_APB1ENR  (*(volatile uint32_t *)(RCC_BASE + 0x40U))
#define I2C_CLOCK (1U << 21)

/* Register accessors can be replaced by the host protocol model. */
#ifndef I2C_READ_REG
#define I2C_READ_REG(offset) (*(volatile uint32_t *)(I2C1_BASE + (offset)))
#endif
#ifndef I2C_WRITE_REG
#define I2C_WRITE_REG(offset, value) \
    (*(volatile uint32_t *)(I2C1_BASE + (offset)) = (value))
#endif

#define CR1   0x00U
#define CR2   0x04U
#define OAR1  0x08U
#define OAR2  0x0CU
#define DR    0x10U
#define SR1   0x14U
#define SR2   0x18U
#define CCR   0x1CU
#define TRISE 0x20U

#define PE    (1U << 0)
#define START (1U << 8)
#define STOP  (1U << 9)
#define ACK   (1U << 10)
#define POS   (1U << 11)
#define SWRST (1U << 15)
#define SB    (1U << 0)
#define ADDR  (1U << 1)
#define BTF   (1U << 2)
#define RXNE  (1U << 6)
#define TXE   (1U << 7)
#define ARLO  (1U << 9)
#define ERRORS ((1U << 8) | ARLO | (1U << 10) | (1U << 11))
#define BUSY  (1U << 1)
#define SCL_PIN 6U
#define SDA_PIN 7U
#define I2C_TIMEOUT_MS 20U
#define I2C_POLL_LIMIT 100000U

static int initialized;

static void cr1_update(uint32_t clear, uint32_t set)
{
    I2C_WRITE_REG(CR1, (I2C_READ_REG(CR1) & ~clear) | set);
}

static uint32_t irq_save(void)
{
#if defined(__arm__) || defined(__thumb__)
    uint32_t state;
    __asm volatile ("mrs %0, primask\n\tcpsid i" : "=r" (state) :: "memory");
    return state;
#else
    return 0;
#endif
}

static void irq_restore(uint32_t state)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("msr primask, %0" :: "r" (state) : "memory");
#else
    (void)state;
#endif
}

static int wait_sr1(uint32_t mask)
{
    uint32_t start = systick_get_tick();
    for (uint32_t poll = 0; poll < I2C_POLL_LIMIT; ++poll)
    {
        uint32_t status = I2C_READ_REG(SR1);
        if (status & ERRORS)
            return -1;
        if ((status & mask) == mask)
            return 0;
        if ((uint32_t)(systick_get_tick() - start) >= I2C_TIMEOUT_MS)
            break;
    }
    return -1;
}

static int wait_idle(void)
{
    uint32_t start = systick_get_tick();
    for (uint32_t poll = 0; poll < I2C_POLL_LIMIT; ++poll)
    {
        if (!(I2C_READ_REG(SR2) & BUSY) && !(I2C_READ_REG(CR1) & STOP))
            return 0;
        if ((uint32_t)(systick_get_tick() - start) >= I2C_TIMEOUT_MS)
            break;
    }
    return -1;
}

static int wait_pin_high(uint8_t pin)
{
    uint32_t start = systick_get_tick();
    for (uint32_t poll = 0; poll < I2C_POLL_LIMIT; ++poll)
    {
        if (gpio_read(GPIOB, pin))
            return 0;
        if ((uint32_t)(systick_get_tick() - start) >= I2C_TIMEOUT_MS)
            break;
    }
    return -1;
}

/* Conservative >=5 us half-period at the reported core clock; intentionally
 * slower than a calibrated bit-bang bus. No interrupt/tick dependency. */
static void recovery_delay(void)
{
    uint32_t cycles = (system_core_clock() / 1000000U + 1U) * 5U;
    while (cycles-- != 0U)
        __asm volatile ("nop");
}

static void pins_af(void)
{
    for (uint8_t pin = SCL_PIN; pin <= SDA_PIN; ++pin)
    {
        gpio_output_type(GPIOB, pin, GPIO_OPEN_DRAIN);
        gpio_pull(GPIOB, pin, GPIO_PULL_UP);
        gpio_speed(GPIOB, pin, GPIO_HIGH_SPEED);
        gpio_af(GPIOB, pin, 4);
        gpio_mode(GPIOB, pin, GPIO_AF);
    }
}

static void peripheral_reset(void)
{
    I2C_WRITE_REG(CR1, 0);
    RCC_APB1RSTR |= I2C_CLOCK;
    (void)RCC_APB1RSTR;
    RCC_APB1RSTR &= ~I2C_CLOCK;
    /* Also reset the I2C state machine explicitly before restoring timing. */
    I2C_WRITE_REG(CR1, SWRST);
    I2C_WRITE_REG(CR1, 0);
    I2C_WRITE_REG(CR2, 16U);
    I2C_WRITE_REG(OAR1, 1U << 14);
    I2C_WRITE_REG(OAR2, 0);
    I2C_WRITE_REG(CCR, 80U);
    I2C_WRITE_REG(TRISE, 17U);
    I2C_WRITE_REG(CR1, PE | ACK);
}

void i2c_init(void)
{
    gpio_clock_enable(GPIOB);
    RCC_APB1ENR |= I2C_CLOCK;
    (void)RCC_APB1ENR;
    I2C_WRITE_REG(CR1, 0);
    pins_af();
    peripheral_reset();
    initialized = 1;
}

int i2c_recover_bus(void)
{
    if (!initialized)
        return -1;
    I2C_WRITE_REG(CR1, 0); /* Release peripheral ownership before GPIO pulses. */
    for (uint8_t pin = SCL_PIN; pin <= SDA_PIN; ++pin)
    {
        gpio_write(GPIOB, pin, 1); /* Preload release, never actively drive high. */
        gpio_output_type(GPIOB, pin, GPIO_OPEN_DRAIN);
        gpio_pull(GPIOB, pin, GPIO_PULL_UP);
        gpio_mode(GPIOB, pin, GPIO_OUTPUT);
    }

    int result = -1;
    if (wait_pin_high(SCL_PIN) != 0)
        goto restore;
    for (unsigned pulse = 0; pulse < 9U && !gpio_read(GPIOB, SDA_PIN); ++pulse)
    {
        gpio_write(GPIOB, SCL_PIN, 0);
        recovery_delay();
        gpio_write(GPIOB, SCL_PIN, 1);
        if (wait_pin_high(SCL_PIN) != 0)
            goto restore;
        recovery_delay();
    }
    if (!gpio_read(GPIOB, SDA_PIN))
        goto restore;

    /* STOP: SDA low while SCL low, release SCL, then release SDA high. */
    gpio_write(GPIOB, SCL_PIN, 0);
    gpio_write(GPIOB, SDA_PIN, 0);
    recovery_delay();
    gpio_write(GPIOB, SCL_PIN, 1);
    if (wait_pin_high(SCL_PIN) != 0)
        goto restore;
    recovery_delay();
    gpio_write(GPIOB, SDA_PIN, 1);
    recovery_delay();
    if (wait_pin_high(SDA_PIN) == 0 && gpio_read(GPIOB, SCL_PIN))
        result = 0;

restore:
    gpio_write(GPIOB, SCL_PIN, 1);
    gpio_write(GPIOB, SDA_PIN, 1);
    pins_af();
    peripheral_reset();
    if (result == 0)
        result = wait_idle();
    if (result != 0)
        I2C_WRITE_REG(CR1, 0); /* A failed clear must leave the lines released. */
    return result;
}

static int prepare(void)
{
    if (!initialized)
        return -1;
    if (!(I2C_READ_REG(CR1) & PE) || !gpio_read(GPIOB, SCL_PIN) ||
        !gpio_read(GPIOB, SDA_PIN) || wait_idle() != 0)
        return i2c_recover_bus(); /* Safe here: no address/data has been sent. */
    cr1_update(POS, ACK);
    return 0;
}

static int abort_transfer(void)
{
    uint32_t errors = I2C_READ_REG(SR1) & ERRORS;
    if (errors & ARLO)
    {
        /* Never emit STOP/GPIO clocks after losing arbitration. */
        peripheral_reset();
        return -1;
    }
    cr1_update(0, STOP);
    I2C_WRITE_REG(SR1, I2C_READ_REG(SR1) & ~ERRORS);
    if (wait_idle() != 0 || !gpio_read(GPIOB, SCL_PIN) || !gpio_read(GPIOB, SDA_PIN))
        (void)i2c_recover_bus();
    else
        peripheral_reset(); /* Discard stale DR/ADDR/ACK/POS state. */
    return -1; /* Never replay an operation that might already have written. */
}

static void clear_addr(void)
{
    (void)I2C_READ_REG(SR1);
    (void)I2C_READ_REG(SR2);
}

static int start_address(uint8_t address, int read)
{
    cr1_update(0, START);
    if (wait_sr1(SB) != 0)
        return -1;
    I2C_WRITE_REG(DR, ((uint32_t)address << 1) | (read ? 1U : 0U));
    return wait_sr1(ADDR); /* Caller clears ADDR at the correct ACK/POS point. */
}

static int write_byte(uint8_t value)
{
    if (wait_sr1(TXE) != 0)
        return -1;
    I2C_WRITE_REG(DR, value);
    return wait_sr1(BTF);
}

static int finish(void)
{
    if (wait_idle() != 0)
        return abort_transfer();
    cr1_update(POS, ACK);
    return 0;
}

int i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    if (dev_addr > 0x7FU || prepare() != 0)
        return -1;
    if (start_address(dev_addr, 0) != 0)
        return abort_transfer();
    clear_addr();
    if (write_byte(reg) != 0 || write_byte(data) != 0)
        return abort_transfer();
    cr1_update(0, STOP);
    return finish();
}

int i2c_read_bytes(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint32_t len)
{
    if (buf == 0 || len == 0U || dev_addr > 0x7FU || prepare() != 0)
        return -1;
    if (start_address(dev_addr, 0) != 0)
        return abort_transfer();
    clear_addr();
    if (write_byte(reg) != 0 || start_address(dev_addr, 1) != 0)
        return abort_transfer();

    uint32_t state = irq_save();
    if (len == 1U)
    {
        cr1_update(ACK | POS, 0);
        clear_addr();
        cr1_update(0, STOP);
        irq_restore(state);
        if (wait_sr1(RXNE) != 0)
            return abort_transfer();
        buf[0] = (uint8_t)I2C_READ_REG(DR);
    }
    else if (len == 2U)
    {
        cr1_update(ACK, POS);
        clear_addr();
        irq_restore(state);
        if (wait_sr1(BTF) != 0)
            return abort_transfer();
        state = irq_save();
        cr1_update(0, STOP);
        buf[0] = (uint8_t)I2C_READ_REG(DR);
        buf[1] = (uint8_t)I2C_READ_REG(DR);
        irq_restore(state);
    }
    else
    {
        cr1_update(POS, ACK);
        clear_addr();
        irq_restore(state);
        uint32_t index = 0;
        while (len - index > 3U)
        {
            if (wait_sr1(RXNE) != 0)
                return abort_transfer();
            buf[index++] = (uint8_t)I2C_READ_REG(DR);
        }
        if (wait_sr1(BTF) != 0)
            return abort_transfer();
        state = irq_save();
        cr1_update(ACK, 0);
        buf[index++] = (uint8_t)I2C_READ_REG(DR);
        irq_restore(state);
        if (wait_sr1(BTF) != 0)
            return abort_transfer();
        state = irq_save();
        cr1_update(0, STOP);
        buf[index++] = (uint8_t)I2C_READ_REG(DR);
        buf[index] = (uint8_t)I2C_READ_REG(DR);
        irq_restore(state);
    }
    return finish();
}

int i2c_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    if (data == 0)
        return -1;
    uint8_t sample;
    int result = i2c_read_bytes(dev_addr, reg, &sample, 1U);
    if (result == 0)
        *data = sample;
    return result;
}
