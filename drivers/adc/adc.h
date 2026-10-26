#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/* =========================================================
 * ADC Channels
 * ========================================================= */

#define ADC_CHANNEL_0     0
#define ADC_CHANNEL_1     1

typedef enum
{
    ADC_OK = 0,
    ADC_ERR_ARGUMENT = -1,
    ADC_ERR_NOT_READY = -2,
    ADC_ERR_TIMEOUT = -3,
    ADC_ERR_OVERRUN = -4
} adc_status_t;

/* =========================================================
 * APIs
 * ========================================================= */

/* PA0/PA1 only; PA2/PA3 remain reserved for USART2.
 * Configure 12-bit, right-aligned, single software-triggered conversions.
 */
void adc_init(void);

/* Blocking, non-reentrant. Initialize SysTick before use (10 ms timeout).
 * A finite polling limit also prevents a hang if SysTick stops.
 * Write *value (0..4095) only on ADC_OK. NULL or channels other than 0/1 fail.
 * Timeout/overrun stops the ADC; call adc_init() before retrying.
 */
adc_status_t adc_read(uint8_t channel, uint16_t *value);

#endif
