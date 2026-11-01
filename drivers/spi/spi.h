#ifndef SPI_H
#define SPI_H

#include <stdint.h>

/* SPI1: PA5 SCK, PA6 MISO, PA7 MOSI (AF5), mode 0, MSB first,
 * 8-bit full duplex, APB2/16 = 1 MHz with the current 16 MHz clock.
 * The caller owns chip select. Blocking, non-reentrant; initialize SysTick first.
 */
void spi1_init(void);

/* NULL tx clocks out 0xFF; NULL rx discards received bytes. At least one
 * buffer and a nonzero length are required. Returns 0 on success, -1 on
 * argument, peripheral or timeout errors. On error rx may be partially filled;
 * deassert CS before retrying. Waits have a finite fallback if SysTick stops.
 */
int spi1_transfer(const uint8_t *tx, uint8_t *rx, uint32_t len);

#endif
