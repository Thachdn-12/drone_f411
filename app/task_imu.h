#ifndef TASK_IMU_H
#define TASK_IMU_H
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float roll;
    float pitch;

} imu_state_t;

void task_imu_init(void);
void task_imu_set_available(bool available);
bool task_imu_healthy(void);
uint32_t task_imu_last_sample_ms(void);

/* One sample/update per call; the caller owns cadence (TIM2 dispatcher: 10 ms).
 * Blocking I2C; foreground context only. A failed read keeps the last state.
 */
void task_imu_run(void);

const imu_state_t* task_imu_get(void);

#endif
