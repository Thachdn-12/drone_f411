#include "uart.h"
#include "gpio.h"
#include "systick.h"
#include <stdarg.h>

/* =========================================================
 * Base Addresses
 * ========================================================= */

#define RCC_BASE            0x40023800UL
#define GPIOA_BASE          0x40020000UL
#define USART2_BASE         0x40004400UL

/* =========================================================
 * RCC
 * ========================================================= */

#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* =========================================================
 * USART2 Registers
 * ========================================================= */

#define USART2_SR           (*(volatile uint32_t*)(USART2_BASE + 0x00))
#define USART2_DR           (*(volatile uint32_t*)(USART2_BASE + 0x04))
#define USART2_BRR          (*(volatile uint32_t*)(USART2_BASE + 0x08))
#define USART2_CR1          (*(volatile uint32_t*)(USART2_BASE + 0x0C))

/* =========================================================
 * Bit Definitions
 * ========================================================= */

#define USART_SR_RXNE       (1 << 5)
#define USART_SR_TXE        (1 << 7)

#define USART_CR1_UE        (1 << 13)
#define USART_CR1_TE        (1 << 3)
#define USART_CR1_RE        (1 << 2)

/* =========================================================
 * Local Helpers
 * ========================================================= */

static void uart_print_uint(uint32_t value)
{
    char buf[16];
    int i = 0;

    if (value == 0)
    {
        uart_write_char('0');
        return;
    }

    while (value > 0)
    {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i--)
    {
        uart_write_char(buf[i]);
    }
}

static void uart_print_int(int32_t value)
{
    if (value < 0)
    {
        uart_write_char('-');
        value = -value;
    }

    uart_print_uint((uint32_t)value);
}

static void uart_print_hex(uint32_t value)
{
    char hex[] = "0123456789ABCDEF";

    uart_write_string("0x");

    for (int i = 28; i >= 0; i -= 4)
    {
        uart_write_char(
            hex[(value >> i) & 0xF]
        );
    }
}

/* =========================================================
 * UART Init
 * ========================================================= */

void uart_init(void)
{
    /*
     * Enable clocks
     */

    RCC_AHB1ENR |= (1 << 0);   /* GPIOA */
    RCC_APB1ENR |= (1 << 17);  /* USART2 */

    /* =====================================================
     * PA2 -> USART2_TX
     * PA3 -> USART2_RX
     * AF7
     * ===================================================== */

    /* Alternate function mode */
    gpio_mode(GPIOA, 2, GPIO_AF);
    gpio_mode(GPIOA, 3, GPIO_AF);

    /* High speed */
    gpio_speed(GPIOA, 2, GPIO_HIGH_SPEED);
    gpio_speed(GPIOA, 3, GPIO_HIGH_SPEED);

    /* AF7 */
    gpio_af(GPIOA, 2, 0x7);
    gpio_af(GPIOA, 3, 0x7);

    /* =====================================================
     * USART Configuration
     * APB1 = 50MHz
     * Baudrate = 115200
     * ===================================================== */

    /*
     * 50MHz / 115200
     *
     * USARTDIV ≈ 434
     */

    USART2_BRR = 434;

    /* Enable TX + RX */
    USART2_CR1 |= USART_CR1_TE;
    USART2_CR1 |= USART_CR1_RE;

    /* Enable USART */
    USART2_CR1 |= USART_CR1_UE;
}

/* =========================================================
 * Write Character
 * ========================================================= */

void uart_write_char(char c)
{
    while (!(USART2_SR & USART_SR_TXE))
    {
    }

    USART2_DR = c;
}

/* =========================================================
 * Write String
 * ========================================================= */

void uart_write_string(const char *str)
{
    while (*str)
    {
        uart_write_char(*str++);
    }
}

/* =========================================================
 * Read Character
 * ========================================================= */

char uart_read_char(void)
{
    while (!(USART2_SR & USART_SR_RXNE))
    {
    }

    return (char)USART2_DR;
}

/* =========================================================
 * Minimal printf
 * Supported:
 * %c
 * %s
 * %d
 * %u
 * %x
 * ========================================================= */

void uart_printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++;

            switch (*fmt)
            {
                case 'c':
                {
                    char c =
                        (char)va_arg(args, int);

                    uart_write_char(c);
                    break;
                }

                case 's':
                {
                    char *str =
                        va_arg(args, char *);

                    uart_write_string(str);
                    break;
                }

                case 'd':
                {
                    int value =
                        va_arg(args, int);

                    uart_print_int(value);
                    break;
                }

                case 'u':
                {
                    uint32_t value =
                        va_arg(args, uint32_t);

                    uart_print_uint(value);
                    break;
                }

                case 'x':
                {
                    uint32_t value =
                        va_arg(args, uint32_t);

                    uart_print_hex(value);
                    break;
                }

                case '%':
                {
                    uart_write_char('%');
                    break;
                }

                default:
                {
                    uart_write_char('?');
                    break;
                }
            }
        }
        else
        {
            uart_write_char(*fmt);
        }

        fmt++;
    }

    va_end(args);
}