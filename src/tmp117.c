// project files
#include "tmp117.h"
#include "i2c_operations.h"

#include <stdio.h>

uint16_t tmp117_read_temperature(i2c_inst_t *i2c, const uint addr) {
    static uint8_t buf[2];
    i2c_reg_read(i2c, addr, TMP117_TEMP_RESULT, buf, 2);
    return((buf[0] << 8) | buf[1]); 
}

uint16_t tmp117_read_id(i2c_inst_t *i2c, const uint addr) {
    //static uint8_t buf[2];
    uint8_t buf[2];
    //i2c_reg_read(i2c, addr, TMP117_DEVICE_ID, buf, 2);
    uint16_t data16 = ((buf[0] << 8) | buf[1]); 
    return data16 & 0x0fff;
}

float tmp117_temperature_to_celsius(uint16_t data) {
    int16_t temp = data;
    return temp * 0.0078125;
}
