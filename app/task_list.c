#include "task_list.h"
#include "drone_config.h"
#include "timer.h"
#include "task_imu.h"
#include "task_baro.h"
#include "uart.h"

_Static_assert(DRONE_IMU_PERIOD_TICKS > 0U, "IMU period must be nonzero");
_Static_assert(DRONE_LOG_PERIOD_TICKS > 0U, "Log period must be nonzero");
_Static_assert(DRONE_BARO_PERIOD_TICKS > 0U, "Barometer period must be nonzero");

static timer_periodic_t imu_periodic;
static timer_periodic_t baro_periodic;
#if !DRONE_GCS_ENABLED
static timer_periodic_t log_periodic;
#endif

void task_list_init(void)
{
    uint32_t start = timer_get_tick();
    (void)timer_periodic_start(&imu_periodic, DRONE_IMU_PERIOD_TICKS, start);
    (void)timer_periodic_start(&baro_periodic, DRONE_BARO_PERIOD_TICKS, start);
#if !DRONE_GCS_ENABLED
    (void)timer_periodic_start(&log_periodic, DRONE_LOG_PERIOD_TICKS, start);
#endif
}

void task_list_run(void)
{
    if (timer_periodic_poll(&imu_periodic, timer_get_tick()) != 0U)
        task_imu_run();

    /* Read TIM2 again: sensor calls may have consumed one or more ticks. */
    if (timer_periodic_poll(&baro_periodic, timer_get_tick()) != 0U)
        task_baro_sample();

#if !DRONE_GCS_ENABLED
    if (timer_periodic_poll(&log_periodic, timer_get_tick()) != 0U)
    {
        const imu_state_t *imu = task_imu_get();
        uart_printf("R=%d P=%d\r\n", (int)imu->roll, (int)imu->pitch);
    }
#endif
}
