#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

typedef enum
{
    BMP280_OK = 0,
    BMP280_ERR_ARGUMENT = -1,
    BMP280_ERR_IO = -2,
    BMP280_ERR_ID = -3,
    BMP280_ERR_CALIBRATION = -4,
    BMP280_ERR_NOT_READY = -5,
    BMP280_ERR_DATA = -6,
    BMP280_ERR_TIMEOUT = -7
} bmp280_status_t;

typedef struct
{
    int32_t temperature_centi_c; /* 2508 = 25.08 degrees C. */
    uint32_t pressure_pa;
} bmp280_sample_t;

/* Single BMP280 on SPI1, CS=PA4, 4-wire mode. Call after spi1_init() and
 * systick_init(). Requires a running SysTick for startup delays. Configures
 * normal mode, temperature x2, pressure x16, filter x4, standby 0.5 ms.
 * Blocking and non-reentrant. Call again to recover a failed initialization.
 */
bmp280_status_t bmp280_init(void);

/* Reads pressure and temperature in one burst for a coherent sample.
 * Only BMP280_OK updates the output. */
bmp280_status_t bmp280_read(bmp280_sample_t *sample);

#endif
