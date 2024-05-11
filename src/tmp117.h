#ifndef TMP117_H_
#define TMP117_H_

// project files
#include "i2c_operations.h"

enum TMP117_Register {
  TMP117_TEMP_RESULT = 0X00,
  TMP117_CONFIGURATION = 0x01,
  TMP117_T_HIGH_LIMIT = 0X02,
  TMP117_T_LOW_LIMIT = 0X03,
  TMP117_EEPROM_UL = 0X04,
  TMP117_EEPROM1 = 0X05,
  TMP117_EEPROM2 = 0X06,
  TMP117_TEMP_OFFSET = 0X07,
  TMP117_EEPROM3 = 0X08,
  TMP117_DEVICE_ID = 0X0F
};

// I2C address
static const uint8_t TMP117_1_ADDR = 0b1001000; // 1001000x, 0x48
static const uint8_t TMP117_2_ADDR = 0b1001001; // 1001001x, 0x49
static const uint8_t TMP117_3_ADDR = 0b1001010; // 1001010x, 0xA0
static const uint8_t TMP117_4_ADDR = 0b1001011; // 1001011x, 0xA1

// Device ID
// (same as addr but with shift))
#define TMP117_DeviceID1 0x48 << 1 //	GND
#define TMP117_DeviceID2 0x49 << 1 //	Vcc
#define TMP117_DeviceID3 0x4A << 1 //	SDA
#define TMP117_DeviceID4 0x4B << 1 //	SCL

uint16_t tmp117_read_temperature(i2c_inst_t *i2c, const uint addr);
uint16_t tmp117_read_id(i2c_inst_t *i2c, const uint addr);
float tmp117_temperature_to_celsius(uint16_t data);

#endif /* TMP117_H_ */