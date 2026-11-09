#include "task_gcs.h"
#include "gcs_link.h"
#include "drone_config.h"
#include "board_config.h"
#include "systick.h"
#include "uart.h"
#include "pwm.h"
#include "task_imu.h"
#include "task_baro.h"
#include <math.h>

static uint32_t last_poll, last_telemetry;
static uart_stats_t previous_stats;
static bool tx_failed;
static float reference_pressure_pa;

bool gcs_physical_bench_enable(void)
{
    return gpio_read(BOARD_BENCH_PORT, BOARD_BENCH_PIN) == 0U;
}

bool gcs_esc_write_us(uint8_t index, uint16_t pulse_us)
{
    /* Recheck the physical gate at every write, including ramp updates. */
    return index < 4U && gcs_physical_bench_enable() &&
           pwm_write_us((uint8_t)(index + 1U), pulse_us);
}

bool gcs_esc_disable_outputs(void) { return pwm_disable_outputs(); }

void gcs_uart_send(const char *data, size_t length)
{
    /* uart_write copies the complete frame or rejects it; never send a prefix. */
    if (length > UINT32_MAX || uart_write((const uint8_t *)data, (uint32_t)length) != UART_OK)
        tx_failed = true;
}

static void publish(uint32_t now)
{
    bool imu_ok = task_imu_healthy() &&
        (uint32_t)(now - task_imu_last_sample_ms()) <= DRONE_GCS_IMU_STALE_MS;
    const bmp280_sample_t *baro = task_baro_get();
    bool baro_ok = baro &&
        baro->pressure_pa >= 30000U && baro->pressure_pa <= 110000U &&
        (uint32_t)(now - task_baro_last_sample_ms()) <= DRONE_GCS_BARO_STALE_MS;
    /* Yaw remains unavailable; altitude is relative to the first valid pressure. */
    gcs_telemetry_t t = { .roll=NAN, .pitch=NAN, .yaw=NAN,
        .ax=NAN, .ay=NAN, .az=NAN, .gx=NAN, .gy=NAN, .gz=NAN,
        .pressure=NAN, .temperature=NAN, .altitude=NAN };
    if (imu_ok)
    {
        const imu_state_t *imu = task_imu_get();
        t.roll = imu->roll; t.pitch = imu->pitch;
        t.ax = imu->ax * 9.80665f; t.ay = imu->ay * 9.80665f; t.az = imu->az * 9.80665f;
        t.gx = imu->gx; t.gy = imu->gy; t.gz = imu->gz;
    }
    if (baro_ok)
    {
        t.pressure = (float)baro->pressure_pa;
        t.temperature = (float)baro->temperature_centi_c / 100.0f;
        if (reference_pressure_pa == 0.0f) reference_pressure_pa = t.pressure;
        /* Standard barometric approximation; local pressure is our zero datum.
         * https://github.com/adafruit/Adafruit_BMP280_Library (readAltitude). */
        t.altitude = 44330.0f * (1.0f - powf(t.pressure / reference_pressure_pa, 0.1903f));
    }
    gcs_set_sensor_health(imu_ok ? GCS_HEALTH_OK : GCS_HEALTH_FAULT,
        baro_ok ? GCS_HEALTH_OK : GCS_HEALTH_FAULT, GCS_CAL_NEEDED);
    gcs_publish_telemetry(&t);
}

void task_gcs_init(void)
{
    gpio_clock_enable(BOARD_BENCH_PORT);
    gpio_pull(BOARD_BENCH_PORT, BOARD_BENCH_PIN, GPIO_PULL_UP);
    gpio_mode(BOARD_BENCH_PORT, BOARD_BENCH_PIN, GPIO_INPUT);
    tx_failed = false;
    reference_pressure_pa = 0.0f;
    uart_get_stats(&previous_stats);
    last_poll = last_telemetry = systick_get_tick();
    gcs_init(last_poll);
    (void)gcs_configure_esc(DRONE_GCS_ESC_MIN_US, DRONE_GCS_ESC_MAX_US, DRONE_GCS_ESC_RAMP);
}

void task_gcs_run(void)
{
    uint32_t now = systick_get_tick();
    uart_stats_t stats;
    uart_get_stats(&stats);
    if (tx_failed || stats.rx_dropped != previous_stats.rx_dropped ||
        stats.rx_errors != previous_stats.rx_errors ||
        stats.tx_dma_errors != previous_stats.tx_dma_errors)
    {
        gcs_transport_fault();
        tx_failed = false;
    }
    previous_stats = stats;
    /* Safety poll before parsing: an overdue heartbeat cannot be revived first. */
    gcs_poll(now);
    uint8_t bytes[64];
    uint32_t received = 0;
    uart_status_t result = uart_read(bytes, sizeof bytes, &received);
    if (result == UART_OK)
        for (uint32_t i = 0; i < received; ++i)
        {
            gcs_rx_byte(bytes[i], systick_get_tick());
            if (tx_failed) break;
        }
    else if (result != UART_ERR_WOULD_BLOCK) tx_failed = true;
    now = systick_get_tick();
    if ((uint32_t)(now - last_poll) >= DRONE_GCS_POLL_MS)
    { last_poll = now; gcs_poll(now); }
    if ((uint32_t)(now - last_telemetry) >= DRONE_GCS_TELEMETRY_MS)
    { last_telemetry = now; publish(now); }
    if (tx_failed) { gcs_transport_fault(); tx_failed = false; }
}
