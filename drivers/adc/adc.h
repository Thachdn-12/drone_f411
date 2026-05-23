#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/* =========================================================
 * ADC Channels
 * ========================================================= */

#define ADC_CHANNEL_0     0
#define ADC_CHANNEL_1     1
#define ADC_CHANNEL_2     2
#define ADC_CHANNEL_3     3
#define ADC_CHANNEL_4     4
#define ADC_CHANNEL_5     5
#define ADC_CHANNEL_6     6
#define ADC_CHANNEL_7     7
#define ADC_CHANNEL_8     8
#define ADC_CHANNEL_9     9

/* =========================================================
 * APIs
 * ========================================================= */

void adc_init(void);

uint16_t adc_read(uint8_t channel);

#endif