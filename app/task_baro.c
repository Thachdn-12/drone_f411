#include "task_baro.h"
#include "systick.h"
#include "uart.h"

static int active;
static uint32_t last_read_ms;

bmp280_status_t task_baro_init(void)
{
    bmp280_status_t status = bmp280_init();
    active = status == BMP280_OK;
    last_read_ms = systick_get_tick();
    return status;
}

void task_baro_run(void)
{
    uint32_t now = systick_get_tick();
    if (!active || (uint32_t)(now - last_read_ms) < 1000U)
        return;
    last_read_ms = now;
    bmp280_sample_t sample;
    bmp280_status_t status = bmp280_read(&sample);
    if (status == BMP280_OK)
        uart_printf("BMP280: T_centiC=%d P_Pa=%u\r\n",
                    (int)sample.temperature_centi_c, (unsigned int)sample.pressure_pa);
    else
        uart_printf("BMP280: read failed (%d)\r\n", (int)status);
}
