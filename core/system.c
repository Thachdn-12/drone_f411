#include "system.h"
#include "board_config.h"

/* SCB CPACR */
#define SCB_CPACR (*(volatile uint32_t*)0xE000ED88)
#define RCC_CR (*(volatile uint32_t*)0x40023800)

static uint32_t g_system_clock = SYSTEM_CORE_CLOCK_HZ;

void system_init(void)
{
    /* Keep the reset HSI clock and undivided buses, with this board's measured
     * trim. A slow HSI made RX reject HELLO despite working TX telemetry. */
    RCC_CR = (RCC_CR & ~(31U << 3)) | (BOARD_HSI_TRIM << 3);

    /*
     * Enable FPU
     *
     * CP10
     * CP11
     */

    SCB_CPACR |= (0xFU << 20);
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");
}

uint32_t system_core_clock(void)
{
    return g_system_clock;
}
