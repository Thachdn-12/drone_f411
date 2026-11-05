#include "gpio.h"

#define RCC_APB2ENR    (*(volatile uint32_t *)(RCC_BASE + 0x44U))
#define SYSCFG_EXTICR  ((volatile uint32_t *)0x40013808UL)
#define EXTI_IMR      (*(volatile uint32_t *)0x40013C00UL)
#define EXTI_EMR      (*(volatile uint32_t *)0x40013C04UL)
#define EXTI_RTSR     (*(volatile uint32_t *)0x40013C08UL)
#define EXTI_FTSR     (*(volatile uint32_t *)0x40013C0CUL)
#define EXTI_PR       (*(volatile uint32_t *)0x40013C14UL)
#define NVIC_ISER     ((volatile uint32_t *)0xE000E100UL)
#define NVIC_ICER     ((volatile uint32_t *)0xE000E180UL)
#define NVIC_ICPR     ((volatile uint32_t *)0xE000E280UL)
#define NVIC_IPR      ((volatile uint8_t *)0xE000E400UL)

typedef struct
{
    gpio_t *gpio;
    gpio_exti_callback_t callback;
    void *context;
} gpio_exti_line_t;

static gpio_exti_line_t lines[16];

static int port_index(gpio_t *gpio)
{
    if (gpio == GPIOA) return 0;
    if (gpio == GPIOB) return 1;
    if (gpio == GPIOC) return 2;
    if (gpio == GPIOD) return 3;
    if (gpio == GPIOE) return 4;
    return -1;
}

static uint32_t irq_save(void)
{
#if defined(__arm__) || defined(__thumb__)
    uint32_t state;
    __asm volatile ("mrs %0, primask\n\tcpsid i" : "=r" (state) :: "memory");
    return state;
#else
    return 0; /* Host register tests run in a single thread. */
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

static uint32_t irq_number(uint8_t pin)
{
    return pin < 5U ? 6U + pin : (pin < 10U ? 23U : 40U);
}

static uint32_t irq_lines(uint8_t pin)
{
    return pin < 5U ? 1U << pin : (pin < 10U ? 0x03E0U : 0xFC00U);
}

gpio_exti_status_t gpio_exti_attach(gpio_t *gpio, uint8_t pin,
                                    gpio_pull_t pull, gpio_exti_trigger_t trigger,
                                    gpio_exti_callback_t callback, void *context)
{
    int port = port_index(gpio);
    if (port < 0 || pin >= 16U || (uint32_t)pull > GPIO_PULL_DOWN ||
        (uint32_t)trigger < GPIO_EXTI_RISING || (uint32_t)trigger > GPIO_EXTI_BOTH ||
        callback == 0)
        return GPIO_EXTI_ERR_ARGUMENT;

    uint32_t state = irq_save();
    if (lines[pin].gpio != 0 && lines[pin].gpio != gpio)
    {
        irq_restore(state);
        return GPIO_EXTI_ERR_BUSY;
    }

    uint32_t bit = 1U << pin;
    uint32_t irq = irq_number(pin);
    uint32_t shift = (pin % 4U) * 4U;
    gpio_clock_enable(gpio);
    RCC_APB2ENR |= 1U << 14; /* SYSCFG clock. */
    (void)RCC_APB2ENR;

    EXTI_IMR &= ~bit;
    EXTI_EMR &= ~bit; /* Interrupt only, no event generation. */
    gpio_mode(gpio, pin, GPIO_INPUT);
    gpio_pull(gpio, pin, pull);
    SYSCFG_EXTICR[pin / 4U] =
        (SYSCFG_EXTICR[pin / 4U] & ~(15U << shift)) | ((uint32_t)port << shift);
    EXTI_RTSR = (EXTI_RTSR & ~bit) | ((trigger & GPIO_EXTI_RISING) ? bit : 0U);
    EXTI_FTSR = (EXTI_FTSR & ~bit) | ((trigger & GPIO_EXTI_FALLING) ? bit : 0U);
    lines[pin].gpio = gpio;
    lines[pin].callback = callback;
    lines[pin].context = context;
    EXTI_PR = bit; /* W1C: never read-modify-write the pending register. */

    /* Do not clear a shared NVIC pending bit while a peer line is enabled. */
    if ((EXTI_IMR & irq_lines(pin)) == 0U)
        NVIC_ICPR[irq / 32U] = 1U << (irq % 32U);
    NVIC_IPR[irq] = 0xF0U; /* Four priority bits; lowest urgency. */
    EXTI_IMR |= bit;
    NVIC_ISER[irq / 32U] = 1U << (irq % 32U);
    irq_restore(state);
    return GPIO_EXTI_OK;
}

gpio_exti_status_t gpio_exti_detach(gpio_t *gpio, uint8_t pin)
{
    if (port_index(gpio) < 0 || pin >= 16U)
        return GPIO_EXTI_ERR_ARGUMENT;

    uint32_t state = irq_save();
    if (lines[pin].gpio != gpio)
    {
        irq_restore(state);
        return GPIO_EXTI_ERR_NOT_ATTACHED;
    }

    uint32_t bit = 1U << pin;
    uint32_t irq = irq_number(pin);
    EXTI_IMR &= ~bit;
    EXTI_EMR &= ~bit;
    EXTI_RTSR &= ~bit;
    EXTI_FTSR &= ~bit;
    EXTI_PR = bit;
    lines[pin].callback = 0;
    lines[pin].context = 0;
    lines[pin].gpio = 0;
    if ((EXTI_IMR & irq_lines(pin)) == 0U)
    {
        NVIC_ICER[irq / 32U] = 1U << (irq % 32U);
        NVIC_ICPR[irq / 32U] = 1U << (irq % 32U);
    }
    irq_restore(state);
    return GPIO_EXTI_OK;
}

static void dispatch(uint32_t group)
{
    uint32_t pending = EXTI_PR & EXTI_IMR & group;
    for (uint8_t pin = 0; pin < 16U; ++pin)
    {
        uint32_t bit = 1U << pin;
        if ((pending & bit) == 0U)
            continue;
        /* A prior callback may have detached another line in this group. */
        if ((EXTI_IMR & bit) == 0U)
            continue;
        EXTI_PR = bit; /* Acknowledge before callback; preserve later edges. */
        gpio_exti_line_t line = lines[pin];
        if (line.callback != 0)
            line.callback(line.gpio, pin, line.context);
    }
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("dsb" ::: "memory"); /* Complete peripheral writes before return. */
#endif
}

void EXTI0_IRQHandler(void) { dispatch(1U << 0); }
void EXTI1_IRQHandler(void) { dispatch(1U << 1); }
void EXTI2_IRQHandler(void) { dispatch(1U << 2); }
void EXTI3_IRQHandler(void) { dispatch(1U << 3); }
void EXTI4_IRQHandler(void) { dispatch(1U << 4); }
void EXTI9_5_IRQHandler(void) { dispatch(0x03E0U); }
void EXTI15_10_IRQHandler(void) { dispatch(0xFC00U); }
