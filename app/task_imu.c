#include "task_imu.h"

#include "mpu6050.h"
#include "systick.h"


/* =========================================================
 * Configuration
 * ========================================================= */

#define IMU_UPDATE_PERIOD_MS    10U

#define ACCEL_SCALE             16384.0f
#define GYRO_SCALE              131.0f

#define RAD_TO_DEG              57.2957795f

/* =========================================================
 * Local State
 * ========================================================= */

static imu_state_t g_imu;

static uint32_t g_last_update_ms;

/* =========================================================
 * Internal Helpers
 * ========================================================= */

static void imu_compute_attitude(void);

/* =========================================================
 * Init
 * ========================================================= */

void task_imu_init(void)
{
    g_last_update_ms = 0;

    g_imu.ax = 0.0f;
    g_imu.ay = 0.0f;
    g_imu.az = 0.0f;

    g_imu.gx = 0.0f;
    g_imu.gy = 0.0f;
    g_imu.gz = 0.0f;

    g_imu.roll = 0.0f;
    g_imu.pitch = 0.0f;
}

/* =========================================================
 * Run
 * Call periodically from main loop
 * ========================================================= */

void task_imu_run(void)
{
    mpu6050_raw_t raw;

    uint32_t now = systick_get_tick();

    if ((now - g_last_update_ms) < IMU_UPDATE_PERIOD_MS)
    {
        return;
    }

    g_last_update_ms = now;

    if (mpu6050_read_raw(&raw) != 0)
    {
        return;
    }

    /* -------------------------------------
     * Accelerometer (g)
     * ------------------------------------- */

    g_imu.ax = (float)raw.ax / ACCEL_SCALE;
    g_imu.ay = (float)raw.ay / ACCEL_SCALE;
    g_imu.az = (float)raw.az / ACCEL_SCALE;

    /* -------------------------------------
     * Gyroscope (deg/s)
     * ------------------------------------- */

    g_imu.gx = (float)raw.gx / GYRO_SCALE;
    g_imu.gy = (float)raw.gy / GYRO_SCALE;
    g_imu.gz = (float)raw.gz / GYRO_SCALE;

    /* -------------------------------------
     * Attitude
     * ------------------------------------- */

    imu_compute_attitude();
}

/* =========================================================
 * Compute Roll/Pitch from Accelerometer
 * ========================================================= */

static void imu_compute_attitude(void)
{
    g_imu.roll =
        atan2f(
            g_imu.ay,
            g_imu.az
        ) * RAD_TO_DEG;

    g_imu.pitch =
        atan2f(
            -g_imu.ax,
            sqrtf(
                (g_imu.ay * g_imu.ay) +
                (g_imu.az * g_imu.az)
            )
        ) * RAD_TO_DEG;
}

/* =========================================================
 * Getter
 * ========================================================= */

const imu_state_t* task_imu_get(void)
{
    return &g_imu;
}