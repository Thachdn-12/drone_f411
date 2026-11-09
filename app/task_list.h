#ifndef TASK_LIST_H
#define TASK_LIST_H

/* Call after TIM2 and the sensor tasks have been initialized. Resets phases;
 * first releases occur one period later. Only call from foreground context.
 */
void task_list_init(void);

/* Poll releases without waiting. IMU runs before barometer/logging when due.
 * At most one invocation per task per call; missed releases are skipped.
 * Sensor calls remain blocking; UART logs are queued for DMA transmission.
 * This is a cooperative dispatcher.
 */
void task_list_run(void);

#endif
