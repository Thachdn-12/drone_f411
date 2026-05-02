#ifndef I2C_H
#define I2C_H

void i2c_init(void);
void i2c_read(uint8_t addr, uint8_t reg, uint8_t *data);
void i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
#endif