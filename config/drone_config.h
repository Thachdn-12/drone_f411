#ifndef DRONE_CONFIG_H
#define DRONE_CONFIG_H

/* The foreground dispatcher uses TIM2 at 1 kHz; all periods are TIM2 ticks. */
#define DRONE_TIMER_HZ          1000U
#define DRONE_IMU_PERIOD_TICKS  10U
#define DRONE_LOG_PERIOD_TICKS  100U
#define DRONE_BARO_PERIOD_TICKS 1000U

/* USART2 is owned by the GCS protocol after initialization. */
#ifndef DRONE_GCS_ENABLED
#define DRONE_GCS_ENABLED       1
#endif
#define DRONE_GCS_POLL_MS       10U
#define DRONE_GCS_TELEMETRY_MS  100U
#define DRONE_GCS_IMU_STALE_MS  100U
#define DRONE_GCS_BARO_STALE_MS 2500U
/* PWM limits require ESC/bench validation. No motor starts merely on ARM. */
#define DRONE_GCS_ESC_MIN_US    1000U
#define DRONE_GCS_ESC_MAX_US    2000U
#define DRONE_GCS_ESC_RAMP      200U

#endif
