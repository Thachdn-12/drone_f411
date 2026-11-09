#ifndef TASK_BARO_H
#define TASK_BARO_H

#include "bmp280.h"
#include <stdbool.h>

bmp280_status_t task_baro_init(void);
/* One read/log per call if initialized; TIM2 dispatcher owns the cadence.
 * Blocking SPI, queued UART logging: call only from foreground context.
 */
void task_baro_sample(void);
/* NULL until a successful sample, and after any failed read. */
const bmp280_sample_t *task_baro_get(void);
uint32_t task_baro_last_sample_ms(void);
void task_baro_set_logging(bool enabled);

/* SysTick-paced 1 Hz wrapper retained for the MPU6050-unavailable boot loop.
 * Use either the dispatcher or this wrapper, not both concurrently.
 */
void task_baro_run(void);

#endif
