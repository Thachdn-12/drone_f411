#include "systick.h"

/* =========================================================
 * Cortex-M SysTick Registers
 * ========================================================= */

#define SYSTICK_BASE       0xE000E010UL

#define SYST_CSR           (*(volatile uint32_t*)(SYSTICK_BASE + 0x00))
#define SYST_RVR           (*(volatile uint32_t*)(SYSTICK_BASE + 0x04))
#define SYST_CVR           (*(volatile uint32_t*)(SYSTICK_BASE + 0x08))

/* =========================================================
 * SysTick CSR Bits
 * ========================================================= */

#define SYSTICK_ENABLE     (1 << 0)
#define SYSTICK_TICKINT    (1 << 1)
#define SYSTICK_CLKSOURCE  (1 << 2)

/* =========================================================
 * Globals
 * ========================================================= */

static volatile uint32_t g_systick_ms = 0;

/* =========================================================
 * SysTick Interrupt Handler
 * ========================================================= */

void SysTick_Handler(void)
{
    g_systick_ms++;
}

/* =========================================================
 * Initialize SysTick
 * ========================================================= */

void systick_init(uint32_t core_clock_hz)
{
    /*
     * Generate interrupt every 1ms
     *
     * reload = (F_CPU / 1000) - 1
     */

    uint32_t reload;

    reload = (core_clock_hz / 1000UL) - 1;

    /* SysTick is 24-bit */
    if (reload > 0xFFFFFF)
    {
        while (1)
        {
        }
    }

    /* Disable SysTick */
    SYST_CSR = 0;

    /* Set reload value */
    SYST_RVR = reload;

    /* Clear current value */
    SYST_CVR = 0;

    /*
     * Enable:
     * - counter
     * - interrupt
     * - processor clock
     */

    SYST_CSR =
        SYSTICK_ENABLE |
        SYSTICK_TICKINT |
        SYSTICK_CLKSOURCE;
}

/* =========================================================
 * Get Tick
 * ========================================================= */

uint32_t systick_get_tick(void)
{
    return g_systick_ms;
}

/* =========================================================
 * Blocking Delay
 * ========================================================= */

void delay_ms(uint32_t ms)
{
    uint32_t start;

    start = systick_get_tick();

    while ((systick_get_tick() - start) < ms)
    {
    }
}