#include "uart.h"
#include "gpio.h"
#include "systick.h"
#include <stdarg.h>

#define REG32(address) (*(volatile uint32_t *)(address))
#define RCC_APB1ENR REG32(RCC_BASE + 0x40U)
#define USART_SR    REG32(0x40004400UL)
#define USART_DR    REG32(0x40004404UL)
#define USART_BRR   REG32(0x40004408UL)
#define USART_CR1   REG32(0x4000440CUL)
#define USART_CR2   REG32(0x40004410UL)
#define USART_CR3   REG32(0x40004414UL)
#define DMA_HISR   REG32(0x40026004UL)
#define DMA_HIFCR  REG32(0x4002600CUL)
#define DMA_CR     REG32(0x400260A0UL)
#define DMA_NDTR   REG32(0x400260A4UL)
#define DMA_PAR    REG32(0x400260A8UL)
#define DMA_M0AR   REG32(0x400260ACUL)
#define DMA_FCR    REG32(0x400260B4UL)
#define NVIC_ISER  ((volatile uint32_t *)0xE000E100UL)
#define NVIC_ICER  ((volatile uint32_t *)0xE000E180UL)
#define NVIC_ICPR  ((volatile uint32_t *)0xE000E280UL)
#define NVIC_IPR   ((volatile uint8_t *)0xE000E400UL)

#define USART_RXNE (1U << 5)
#define USART_TC   (1U << 6)
#define USART_ERRORS 15U /* PE, FE, NE, ORE: cleared by SR then DR read. */
#define USART_UE   (1U << 13)
#define USART_RXNEIE (1U << 5)
#define USART_DMAT (1U << 7)
#define DMA_EN     1U
#define DMA_TC     (1U << 21)
#define DMA_FIFO_ERROR (1U << 16)
#define DMA_ERRORS ((1U << 18) | (1U << 19))
#define DMA_FLAGS  (DMA_FIFO_ERROR | DMA_ERRORS | DMA_TC | (1U << 20))
#define DMA_CONFIG ((4U << 25) | (1U << 10) | (1U << 6) | (1U << 4) | (1U << 2) | (1U << 1))
#define UART_POLL_LIMIT 100000U

static uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
static volatile uint32_t rx_head, rx_tail, tx_head, tx_tail;
static volatile uint32_t tx_active;
static volatile int ready, tx_fault;
static volatile uart_stats_t counters;

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

static void publish(void)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("dmb" ::: "memory");
#else
    __asm volatile ("" ::: "memory");
#endif
}

static void irq_complete(void)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("dsb" ::: "memory");
#endif
}

/* Caller excludes DMA IRQ. Never modify data inside the active DMA span. */
static void tx_start(void)
{
    if (tx_active != 0U || tx_fault || tx_head == tx_tail)
        return;
    uint32_t tail = tx_tail;
    uint32_t head = tx_head;
    tx_active = head > tail ? head - tail : UART_TX_BUFFER_SIZE - tail;
    /* Arm the stream before exposing the USART's pending TXE request. */
    USART_CR3 &= ~USART_DMAT;
    DMA_HIFCR = DMA_FLAGS; /* W1C only Stream6 flags. */
    DMA_M0AR = (uint32_t)(uintptr_t)&tx_buffer[tail];
    DMA_NDTR = tx_active;
    USART_SR = ~USART_TC; /* Clear TC before a new DMA/wire transaction. */
    publish();
    DMA_CR |= DMA_EN;
    USART_CR3 |= USART_DMAT;
}

uart_status_t uart_init(void)
{
    uint32_t state = irq_save();
    ready = 0;
    NVIC_ICER[0] = 1U << 17; /* DMA1 Stream6. */
    NVIC_ICER[1] = 1U << 6;  /* USART2 IRQ38. */
    gpio_clock_enable(GPIOA);
    RCC_AHB1ENR |= 1U << 21; /* DMA1. */
    RCC_APB1ENR |= 1U << 17;
    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;
    USART_CR3 = 0;
    USART_CR1 = 0;
    DMA_CR &= ~DMA_EN;
    uint32_t polls = UART_POLL_LIMIT;
    while ((DMA_CR & DMA_EN) && polls != 0U)
        --polls;
    if (DMA_CR & DMA_EN)
    {
        irq_restore(state);
        return UART_ERR_TIMEOUT;
    }

    gpio_af(GPIOA, 2, 7);
    gpio_af(GPIOA, 3, 7);
    gpio_output_type(GPIOA, 2, GPIO_PUSH_PULL);
    gpio_output_type(GPIOA, 3, GPIO_PUSH_PULL);
    gpio_speed(GPIOA, 2, GPIO_HIGH_SPEED);
    gpio_speed(GPIOA, 3, GPIO_HIGH_SPEED);
    gpio_pull(GPIOA, 2, GPIO_NO_PULL);
    gpio_pull(GPIOA, 3, GPIO_PULL_UP);
    gpio_mode(GPIOA, 2, GPIO_AF);
    gpio_mode(GPIOA, 3, GPIO_AF);
    USART_CR2 = 0;
    USART_BRR = 139; /* round(16 MHz / 115200); APB1 is undivided. */
    (void)USART_SR;
    (void)USART_DR;
    DMA_CR = DMA_CONFIG;
    /* Direct mode: FEIF can indicate a transient underrun without data loss
     * (RM0383 9.3.18). Keep FEIE off; TE/DME remain fatal errors. */
    DMA_FCR = 0;
    DMA_PAR = 0x40004404UL;
    DMA_NDTR = 0;
    DMA_HIFCR = DMA_FLAGS;
    rx_head = rx_tail = tx_head = tx_tail = tx_active = 0;
    tx_fault = 0;
    counters.rx_dropped = counters.rx_errors = counters.rx_overruns = 0;
    counters.tx_rejected_writes = counters.tx_dma_errors = 0;
    NVIC_ICPR[0] = 1U << 17;
    NVIC_ICPR[1] = 1U << 6;
    NVIC_IPR[17] = 0xF0U;
    NVIC_IPR[38] = 0xE0U;
    ready = 1;
    USART_CR3 = 1U; /* RX error interrupts; tx_start enables TX DMA. */
    USART_CR1 = USART_UE | USART_RXNEIE | (1U << 8) | (1U << 3) | (1U << 2);
    NVIC_ISER[0] = 1U << 17;
    NVIC_ISER[1] = 1U << 6;
    irq_restore(state);
    return UART_OK;
}

void USART2_IRQHandler(void)
{
    if (!ready || !(USART_CR1 & USART_UE))
        return;
    uint32_t status = USART_SR;
    if (status & (USART_RXNE | USART_ERRORS))
    {
        uint8_t byte = (uint8_t)USART_DR; /* SR/DR read clears RXNE and errors. */
        if (status & USART_ERRORS)
        {
            ++counters.rx_errors;
            if (status & (1U << 3))
                ++counters.rx_overruns;
            if (status & USART_RXNE)
                ++counters.rx_dropped;
        }
        else if (status & USART_RXNE)
        {
            uint32_t next = (rx_head + 1U) % UART_RX_BUFFER_SIZE;
            if (next == rx_tail)
                ++counters.rx_dropped;
            else
            {
                rx_buffer[rx_head] = byte;
                publish();
                rx_head = next;
            }
        }
    }
    irq_complete();
}

void DMA1_Stream6_IRQHandler(void)
{
    uint32_t flags = DMA_HISR & DMA_FLAGS;
    if (flags == 0U)
        return;
    DMA_HIFCR = flags;
    if (ready && !tx_fault && tx_active != 0U && (flags & (DMA_ERRORS | DMA_TC)))
    {
        DMA_CR &= ~DMA_EN;
        if ((flags & DMA_ERRORS) || (DMA_CR & DMA_EN) || DMA_NDTR != 0U)
        {
            /* Do not reuse/replay a possibly partially transmitted span. */
            USART_CR3 &= ~USART_DMAT;
            tx_fault = 1;
            ++counters.tx_dma_errors;
        }
        else
        {
            tx_tail = (tx_tail + tx_active) % UART_TX_BUFFER_SIZE;
            tx_active = 0;
            tx_start();
        }
    }
    irq_complete();
}

uart_status_t uart_write(const uint8_t *data, uint32_t len)
{
    if (data == 0 || len == 0U)
        return UART_ERR_ARGUMENT;
    if (!ready)
        return UART_ERR_NOT_READY;
    uart_status_t error = UART_OK;
    uint32_t head = tx_head;
    uint32_t available = (tx_tail + UART_TX_BUFFER_SIZE - head - 1U) % UART_TX_BUFFER_SIZE;
    if (tx_fault)
        error = UART_ERR_IO;
    else if (len >= UART_TX_BUFFER_SIZE)
        error = UART_ERR_TOO_LONG;
    else if (len > available)
        error = UART_ERR_WOULD_BLOCK;
    if (error != UART_OK)
    {
        ++counters.tx_rejected_writes;
        return error;
    }

    /* Copy with interrupts enabled: RX remains responsive. The single producer
     * owns unpublished free slots, even if DMA advances its tail meanwhile. */
    for (uint32_t i = 0; i < len; ++i)
        tx_buffer[(head + i) % UART_TX_BUFFER_SIZE] = data[i];
    uint32_t state = irq_save();
    if (tx_fault)
    {
        ++counters.tx_rejected_writes;
        irq_restore(state);
        return UART_ERR_IO;
    }
    publish();
    tx_head = (head + len) % UART_TX_BUFFER_SIZE;
    tx_start();
    irq_restore(state);
    return UART_OK;
}

uint32_t uart_rx_available(void)
{
    return ready ? (rx_head + UART_RX_BUFFER_SIZE - rx_tail) % UART_RX_BUFFER_SIZE : 0U;
}

uart_status_t uart_read(uint8_t *data, uint32_t capacity, uint32_t *received)
{
    if (received == 0)
        return UART_ERR_ARGUMENT;
    *received = 0;
    if (data == 0 || capacity == 0U)
        return UART_ERR_ARGUMENT;
    if (!ready)
        return UART_ERR_NOT_READY;
    uint32_t count = uart_rx_available();
    if (count == 0U)
        return UART_ERR_WOULD_BLOCK;
    if (count > capacity)
        count = capacity;
    uint32_t tail = rx_tail;
    publish();
    for (uint32_t i = 0; i < count; ++i)
        data[i] = rx_buffer[(tail + i) % UART_RX_BUFFER_SIZE];
    publish();
    rx_tail = (tail + count) % UART_RX_BUFFER_SIZE;
    *received = count;
    return UART_OK;
}

void uart_get_stats(uart_stats_t *stats)
{
    if (stats == 0)
        return;
    uint32_t state = irq_save();
    *stats = counters;
    irq_restore(state);
}

uart_status_t uart_read_timeout(uint8_t *byte, uint32_t timeout_ms)
{
    uint32_t received;
    uart_status_t status = uart_read(byte, 1U, &received);
    if (status != UART_ERR_WOULD_BLOCK)
        return status;
    if (timeout_ms == 0U)
        return UART_ERR_TIMEOUT;
    uint32_t start = systick_get_tick();
    for (uint32_t poll = 0; poll < UART_POLL_LIMIT; ++poll)
    {
        status = uart_read(byte, 1U, &received);
        if (status != UART_ERR_WOULD_BLOCK)
            return status;
        if ((uint32_t)(systick_get_tick() - start) >= timeout_ms)
            break;
    }
    return UART_ERR_TIMEOUT;
}

uart_status_t uart_flush(uint32_t timeout_ms)
{
    if (!ready)
        return UART_ERR_NOT_READY;
    uint32_t start = systick_get_tick();
    for (uint32_t poll = 0; poll < UART_POLL_LIMIT; ++poll)
    {
        if (tx_fault)
            return UART_ERR_IO;
        if (tx_head == tx_tail && tx_active == 0U && (USART_SR & USART_TC))
            return UART_OK;
        if (timeout_ms == 0U || (uint32_t)(systick_get_tick() - start) >= timeout_ms)
            break;
    }
    return UART_ERR_TIMEOUT;
}

/* Format completely before enqueueing, so overflow never sends half a log. */
typedef struct
{
    uint8_t data[UART_PRINTF_SIZE];
    uint32_t len;
} uart_format_t;

static int append_char(uart_format_t *out, char value)
{
    if (out->len >= sizeof out->data)
        return -1;
    out->data[out->len++] = (uint8_t)value;
    return 0;
}

static int append_string(uart_format_t *out, const char *value)
{
    if (value == 0)
        value = "(null)";
    while (*value)
        if (append_char(out, *value++) != 0)
            return -1;
    return 0;
}

static int append_uint(uart_format_t *out, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do
    {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U)
        if (append_char(out, digits[--count]) != 0)
            return -1;
    return 0;
}

static uart_status_t format_write(const char *fmt, va_list args)
{
    if (fmt == 0)
        return UART_ERR_ARGUMENT;
    uart_format_t out;
    out.len = 0;
    while (*fmt)
    {
        int result;
        if (*fmt != '%')
            result = append_char(&out, *fmt++);
        else
        {
            ++fmt;
            if (*fmt == '\0')
                return UART_ERR_ARGUMENT;
            switch (*fmt++)
            {
                case 'c': result = append_char(&out, (char)va_arg(args, int)); break;
                case 's': result = append_string(&out, va_arg(args, const char *)); break;
                case 'd':
                {
                    int32_t value = va_arg(args, int);
                    result = value < 0 ? append_char(&out, '-') : 0;
                    if (result == 0)
                        result = append_uint(&out, value < 0 ? 0U - (uint32_t)value : (uint32_t)value);
                    break;
                }
                case 'u': result = append_uint(&out, va_arg(args, unsigned int)); break;
                case 'x':
                {
                    uint32_t value = va_arg(args, unsigned int);
                    result = append_string(&out, "0x");
                    for (unsigned digit = 0; digit < 8U && result == 0; ++digit)
                        result = append_char(&out, "0123456789ABCDEF"[(value >> (28U - digit * 4U)) & 15U]);
                    break;
                }
                case '%': result = append_char(&out, '%'); break;
                default: result = append_char(&out, '?'); break;
            }
        }
        if (result != 0)
        {
            ++counters.tx_rejected_writes;
            return UART_ERR_TOO_LONG;
        }
    }
    return out.len == 0U ? UART_OK : uart_write(out.data, out.len);
}

uart_status_t uart_printf_try(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    uart_status_t status = format_write(fmt, args);
    va_end(args);
    return status;
}

void uart_write_char(char c)
{
    (void)uart_write((const uint8_t *)&c, 1U);
}

void uart_write_string(const char *str)
{
    if (str == 0)
        return;
    uint32_t len = 0;
    while (len < UART_TX_BUFFER_SIZE && str[len] != '\0')
        ++len;
    if (len != 0U)
        (void)uart_write((const uint8_t *)str, len);
}

char uart_read_char(void)
{
    uint8_t byte = 0;
    (void)uart_read_timeout(&byte, UART_DEFAULT_TIMEOUT_MS);
    return (char)byte;
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void)format_write(fmt, args);
    va_end(args);
}
