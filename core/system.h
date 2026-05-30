#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

/* =========================================================
 * Core Clock
 * ========================================================= */

#define SYSTEM_CORE_CLOCK_HZ 100000000UL

/* =========================================================
 * System APIs
 * ========================================================= */

void system_init(void);

uint32_t system_core_clock(void);

#endif