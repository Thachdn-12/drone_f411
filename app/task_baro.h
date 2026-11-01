#ifndef TASK_BARO_H
#define TASK_BARO_H

#include "bmp280.h"

bmp280_status_t task_baro_init(void);
void task_baro_run(void);

#endif
