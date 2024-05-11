// project files
#include "tmp117.h"
#include "i2c_operations.h"
#include <stdint.h>

/* 
 * Read the temperature from the TMP117 sensor
 * 
 * @param i2c I2C instance
 * @param addr I2C address of the TMP117 sensor
 * @return temperature in bit with a resolution of 0.0078125 degrees Celsius
 */
uint16_t tmp117_read_temperature_in_bits(i2c_inst_t *i2c, const uint addr) {
    static uint8_t buf[2];
    i2c_reg_read(i2c, addr, TMP117_TEMP_RESULT, buf, 2);
    return((buf[0] << 8) | buf[1]); 
}

/* 
 * Read the temperature from the TMP117 sensor in Celsius
 * 
 * @param i2c I2C instance
 * @param addr I2C address of the TMP117 sensor
 * @return temperature in Celsius
 */
float tmp117_read_temperature_in_celsius(i2c_inst_t *i2c, const uint addr) {
    float temperature = tmp117_read_temperature_in_bits(i2c, addr);
    temperature = tmp117_temperature_to_celsius(temperature);
    return temperature;
}

/* 
 * Read the device ID from the TMP117 sensor
 * 
 * @param i2c I2C instance
 * @param addr I2C address of the TMP117 sensor
 * @return device ID, 0 if no communication
 */
uint16_t tmp117_read_id(i2c_inst_t *i2c, const uint addr) {
    //static uint8_t buf[2];
    uint8_t buf[2];
    i2c_reg_read(i2c, addr, TMP117_DEVICE_ID, buf, 2);
    uint16_t data16 = ((buf[0] << 8) | buf[1]); 
    return data16 & 0x0fff;
}

/* 
 * Convert the TMP117 sensor data to Celsius
 * 
 * @param data TMP117 sensor data in bit
 * @return temperature in Celsius
 */
float tmp117_temperature_to_celsius(uint16_t data) {
    int16_t temp = data;
    return temp * 0.0078125;
}
