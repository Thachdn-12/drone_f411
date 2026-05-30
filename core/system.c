#include "system.h"

/* SCB CPACR */
#define SCB_CPACR (*(volatile uint32_t*)0xE000ED88)

static uint32_t g_system_clock = SYSTEM_CORE_CLOCK_HZ;

void system_init(void)
{
    /*
     * Enable FPU
     *
     * CP10
     * CP11
     */

    SCB_CPACR |= (0xFU << 20);
}

uint32_t system_core_clock(void)
{
    return g_system_clock;
}