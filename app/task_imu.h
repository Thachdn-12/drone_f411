#ifndef TASK_IMU_H
#define TASK_IMU_H

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

void task_imu_run(void);

const imu_state_t* task_imu_get(void);

#endif