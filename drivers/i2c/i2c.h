#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/* I2C1 PB6=SCL / PB7=SDA, AF4 open-drain, 100 kHz at APB1=16 MHz.
 * Single controller, blocking/non-reentrant; foreground only. Supply suitable
 * external pull-ups to 3.3 V. Initialize once before any transaction/recovery.
 * APIs return 0 on success, -1 on invalid input, timeout or hardware failure.
 * Device addresses are unshifted 7-bit values. SysTick is optional: every wait
 * is bounded by both 20 ms (when ticking) and a finite polling budget.
 */

void i2c_init(void);

/* Release SDA with up to nine SCL pulses, issue STOP and reset I2C1. No SysTick
 * delays, no other controller permitted. Always restores AF4/open-drain and
 * releases both outputs. Failure leaves PE disabled; later transactions may
 * attempt recovery again. A hard short/target holding SCL low needs physical
 * intervention. No failed write/read transaction is automatically replayed.
 */
int i2c_recover_bus(void);

int i2c_write_reg(uint8_t dev_addr,
                  uint8_t reg,
                  uint8_t data);

int i2c_read_reg(uint8_t dev_addr,
                 uint8_t reg,
                 uint8_t *data);

int i2c_read_bytes(uint8_t dev_addr,
                   uint8_t reg,
                   uint8_t *buf,
                   uint32_t len);

/* A failed multi-byte read may leave a partial buffer: discard it on error.
 * read_reg only updates its output after a fully completed transaction.
 */

#endif
