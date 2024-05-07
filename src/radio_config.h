#ifndef RADIO_H_
#define RADIO_H_
// #define STRING_LENGTH_UINT8 ((CHAR_BIT * sizeof(uint8_t) - 1) / 3 + 2)
#include "stdint.h"

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
#define t_C_CONFIG_US 1100

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
#define ADDR_RF_CHANNEL   0X00
#define ADDR_RF_POWER     0X01
#define ADDR_RF_DATA_RATE 0X02

// void radio_send_test_messages(void);
/*! Sends a message via radio
 */
void radio_send(void);
void radio_reset(void);
void radio_init(void);
void radio_read_temperature(void);
void radio_uart_read_all(void);
void radio_memory_read_one_byte(uint8_t address);
void radio_memory_configuration(void);

uint8_t wait_config_prompt(void);

#endif /* RADIO_H_ */