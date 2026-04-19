#include <stdint.h>

#define RCC_BASE        0x40023800
#define RCC_CR          (*(volatile uint32_t*)(RCC_BASE + 0x00))
#define RCC_CFGR        (*(volatile uint32_t*)(RCC_BASE + 0x08))

void system_init(void)
{
    /* Ensure HSI ON */
    RCC_CR |= (1 << 0);

    /* Wait HSI ready */
    while (!(RCC_CR & (1 << 1)));

    /* Select HSI as system clock */
    RCC_CFGR &= ~(0x3 << 0);
}