#ifndef GCS_LINK_H
#define GCS_LINK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* v0.4 driver contract: true only when the timer driver actually applied the change.
   Applied is a timer-driver acknowledgement, not an ESC/RPM/waveform measurement. */
bool gcs_physical_bench_enable(void);
bool gcs_esc_write_us(uint8_t motor_index, uint16_t pulse_us);
bool gcs_esc_disable_outputs(void);
void gcs_uart_send(const char *data, size_t length);

typedef struct {
    float roll, pitch, yaw;
    float ax, ay, az, gx, gy, gz;
    float pressure, temperature, altitude;
} gcs_telemetry_t;

typedef enum { GCS_HEALTH_UNKNOWN, GCS_HEALTH_OK, GCS_HEALTH_FAULT } gcs_health_t;
typedef enum { GCS_CAL_UNKNOWN, GCS_CAL_NEEDED, GCS_CAL_RUNNING, GCS_CAL_DONE } gcs_calibration_t;

void gcs_init(uint32_t now_ms);
/* Required after init, while disarmed; use limits/ramp validated for your ESC. */
bool gcs_configure_esc(uint16_t min_us, uint16_t max_us, uint16_t ramp_us_per_second);
void gcs_set_sensor_health(gcs_health_t imu, gcs_health_t barometer, gcs_calibration_t calibration);
void gcs_rx_byte(uint8_t byte, uint32_t now_ms);
void gcs_poll(uint32_t now_ms);
/* Foreground transport fault: stop outputs and require a new hello/heartbeat.
 * Do not emit while handling a failed UART write (avoids recursive sends). */
void gcs_transport_fault(void);
void gcs_publish_telemetry(const gcs_telemetry_t *t);
#endif
