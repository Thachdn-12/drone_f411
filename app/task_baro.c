#include "task_baro.h"
#include "systick.h"
#include "uart.h"

static int active;
static uint32_t last_read_ms;
static uint32_t last_sample_ms;
static bmp280_sample_t latest;
static bool valid;
static bool logging = true;

bmp280_status_t task_baro_init(void)
{
    bmp280_status_t status = bmp280_init();
    active = status == BMP280_OK;
    valid = false;
    last_read_ms = systick_get_tick();
    return status;
}

void task_baro_run(void)
{
    uint32_t now = systick_get_tick();
    if (!active || (uint32_t)(now - last_read_ms) < 1000U)
        return;
    last_read_ms = now;
    task_baro_sample();
}

void task_baro_sample(void)
{
    if (!active)
        return;
    bmp280_sample_t sample;
    bmp280_status_t status = bmp280_read(&sample);
    valid = status == BMP280_OK;
    if (status == BMP280_OK)
    {
        latest = sample;
        last_sample_ms = systick_get_tick();
        if (logging)
        uart_printf("BMP280: T_centiC=%d P_Pa=%u\r\n",
                    (int)sample.temperature_centi_c, (unsigned int)sample.pressure_pa);
    }
    else if (logging)
        uart_printf("BMP280: read failed (%d)\r\n", (int)status);
}

const bmp280_sample_t *task_baro_get(void) { return valid ? &latest : 0; }
uint32_t task_baro_last_sample_ms(void) { return last_sample_ms; }
void task_baro_set_logging(bool enabled) { logging = enabled; }
