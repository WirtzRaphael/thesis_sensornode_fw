#ifndef RADIO_CONFIG_H_
#define RADIO_CONFIG_H_
// #define STRING_LENGTH_UINT8 ((CHAR_BIT * sizeof(uint8_t) - 1) / 3 + 2)
#include "stdint.h"
#include <stdint.h>

/* Timing
 * timing informations from rc1701hp datasheet (v1.14)
 */
// Time from last byte is received from the air until first character is sent on
// the UART
#define t_RX_TXD_US 180

// Minimum time for tTXD, # bytes received x 590 us/char (10 bits at 19.2 kBd +
// 70 us delay per character)
#define t_TXD_MIN_US 590

// Time from last character is sent on the UART until module is in IDLE mode
// (ready for RXD and RX)
#define t_TXD_IDLE_US 900

// Time from last character is received by the UART (including any timeout)
// until CTS is activated
#define t_RXD_CTS_US 20

// Time from last character is received by the UART (including any timeout)
// until the module sends the first byte on the air
#define t_RXD_TX_US 960

// Time from last character is sent on the air until module is in IDLE mode
// (ready for RXD and RX)
#define t_TX_IDLE_US 960

// Time from turning the device off until it is in IDLE mode
#define t_OFF_IDLE_US 3200

// Time from reset until device is in IDLE mode
#define t_RESET_IDLE_US 3000

// Time from entering sleep mode until device is in IDLE mode
#define t_SLEEP_IDLE_US 1280

// Time from CONFIG pin is set low until prompt (“>”)
#define t_CONFIG_PROMPT_US 590

// Delay after channel-byte is sent until prompt (“>”). (For other commands like
// ’M’, ’T’ there is no delay but immediate prompt)
#define t_CHANNEL_CONFIG_US 1100

// In this period the internal flash is programmed. Do not reset, turn the
// module off, or allow any power supply dips in this period as it may cause
// permanent error in the Flash configuration memory. After 0xFF the host should
// wait for the ‘>’ prompt before any further action is done to ensure correct
// re-configuration.
#define t_MEMORY_CONFIG_MS 62

#define t_CONFIG_IDLE_US 1420

// tTX = # bytes to send x 1.67 ms/byte (at 4.8 kbit/s) + 2 bytes preamble, sync
// + 2 bytes address + 2 bytes CRC
#define t_TX_MIN_MS 12000

// Time from end of S command to start of RSSI byte received on UART
#define t_RSSI_MS 20000

/* ADDRESSES
 */
#define NVM_ADDR_RF_CHANNEL      0X00
#define NVM_ADDR_RF_POWER        0X01
#define NVM_ADDR_RF_DATA_RATE    0X02
#define NVM_ADDR_PACKET_END_CHAR 0x11
#define NVM_ADDR_ADDRESS_MODE    0x14
#define NVM_ADDR_CRC             0x15
#define NVM_ADDR_UID             0x19
#define NVM_ADDR_SID             0x1A
#define NVM_ADDR_DID             0x21
#define NVM_ADDR_LED_CONTROL     0X3A
#define NVM_CMD_EXIT             0XFF

#define RADIO_BROADCAST_ADDRESS 0xFF

// void rc232_send_test_messages(void);
/*! Sends a message via radio
 */
void exit_config_state(void);
void rc232_send_string(const char *message);
void rc232_send_test(void);
void rc232_reset(void);
void rc232_init(void);
void rc232_config_destination_address(uint8_t address);
void rc232_config_rf_channel_number(uint8_t channel);
void rc232_config_rf_power(uint8_t power);
void rc232_sleep(void);
void rc232_wakeup(void);
void rc232_get_configuration_memory(void);
void rc232_read_temperature(void);
void rc232_read_voltage(void);
uint8_t rc232_signal_strength_indicator(void);
void rc232_uart_read_all(void);
void rc232_memory_read_one_byte(uint8_t address);
void rc232_memory_write_configuration(void);
void rc232_memory_read_configuration(void);

uint8_t wait_config_prompt(void);
uint8_t check_config_prompt(uint8_t received);

#endif /* RADIO_CONFIG_H_ */