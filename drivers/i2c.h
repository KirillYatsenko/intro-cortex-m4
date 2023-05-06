#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>
#include <stddef.h>

// i2c module #1: PA6(SCL) - PA7(SDA) pins
void i2c_init(uint32_t main_clock_speed, uint32_t i2c_clock);
int i2c_transmit(uint8_t addr, uint8_t *val, size_t size);
int i2c_receive(uint8_t addr, uint8_t *val, size_t size);

#endif
