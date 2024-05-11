
#ifndef I2C_OPERATIONS_H_
#define I2C_OPERATIONS_H_

#include "hardware/i2c.h"

void i2c_operations_test(void);

void i2c_operations_init(uint sda_pin, uint scl_pin);

int i2c_reg_write(i2c_inst_t *i2c, 
                const uint addr, 
                const uint8_t reg, 
                uint8_t *buf,
                const uint8_t nbytes);

int i2c_reg_read(i2c_inst_t *i2c,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes);

#endif /* I2C_OPERATIONS_H_ */