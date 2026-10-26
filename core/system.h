#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

/* =========================================================
 * Core Clock
 * ========================================================= */

/* Reset HSI clock; no PLL or bus prescalers are configured. */
#define SYSTEM_CORE_CLOCK_HZ 16000000UL

/* =========================================================
 * System APIs
 * ========================================================= */

void system_init(void);

uint32_t system_core_clock(void);

#endif
