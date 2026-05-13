#include "uart.h"

#define PERIPH_BASE     0x40000000
#define AHB1_OFFSET     0x00020000
#define APB2_OFFSET     0x00010000

#define AHB1_BASE       (PERIPH_BASE + AHB1_OFFSET)
#define APB2_BASE       (PERIPH_BASE + APB2_OFFSET)

#define GPIOA_BASE      (AHB1_BASE + 0x0000)
#define USART1_BASE     (APB2_BASE + 0x1000)
#define RCC_BASE        (AHB1_BASE + 0x3800)

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t RESERVED0[2];
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t RESERVED2[2];
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
} USART_TypeDef;

#define GPIOA ((GPIO_TypeDef*) GPIOA_BASE)
#define RCC   ((RCC_TypeDef*) RCC_BASE)
#define USART1 ((USART_TypeDef*) USART1_BASE)

void uart_init(uint32_t baudrate) {
    // enable clock GPIOA + USART1
    RCC->AHB1ENR |= (1 << 0);
    RCC->APB2ENR |= (1 << 4);

    // PA9 (TX), PA10 (RX) -> AF mode
    GPIOA->MODER &= ~((3 << 18) | (3 << 20));
    GPIOA->MODER |=  ((2 << 18) | (2 << 20));

    // AF7 (USART1)
    GPIOA->AFR[1] &= ~((0xF << 4) | (0xF << 8));
    GPIOA->AFR[1] |=  ((7 << 4) | (7 << 8));

    // baudrate (assuming 16MHz clock)
    uint32_t uartdiv = 16000000 / baudrate;
    USART1->BRR = uartdiv;

    // enable TX, RX, USART
    USART1->CR1 |= (1 << 3) | (1 << 2) | (1 << 13);
}

void uart_write_char(char c) {
    while (!(USART1->SR & (1 << 7))); // TXE
    USART1->DR = c;
}

void uart_write_string(const char *s) {
    while (*s) {
        uart_write_char(*s++);
    }
}

void uart_write_int(int value) {
    char buf[12];
    int i = 0;

    if (value == 0) {
        uart_write_char('0');
        return;
    }

    if (value < 0) {
        uart_write_char('-');
        value = -value;
    }

    while (value > 0) {
        buf[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i--) {
        uart_write_char(buf[i]);
    }
}

char uart_read_char(void) {
    while (!(USART1->SR & (1 << 5))); // RXNE
    return USART1->DR;
}

void uart_print_uint(uint32_t value)
{
    char buf[11];
    int i = 0;

    if (value == 0) {
        uart_write_char('0');
        return;
    }

    while (value > 0) {
        buf[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i--) {
        uart_write_char(buf[i]);
    }
}