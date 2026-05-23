#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/* =========================================================
 * APIs
 * ========================================================= */

void i2c_init(void);

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

#endif