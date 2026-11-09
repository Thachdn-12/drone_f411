#ifndef TASK_GCS_H
#define TASK_GCS_H
/* Single foreground owner for protocol, UART and PWM hooks. */
void task_gcs_init(void);
void task_gcs_run(void);
#endif
