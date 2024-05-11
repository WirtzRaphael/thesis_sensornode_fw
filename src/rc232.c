/**
 * @file rc232_config.c
 * @author raphael wirtz
 * @brief Configuration and usage of the radiocrafts rc17xxhp moodule with the
 * RC-232 protocol.
 * @version 1
 * @date 2024-05-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "rc232.h"
#include "pico/time.h"
#include "stdio.h"

#include "pico/stdlib.h"
#include "pico_config.h"

#include "McuLog.h"
#include "McuUtility.h"
#include "McuWait.h"

#include "hardware/uart.h"
#include <errno.h>
#include <stdint.h>

#define RADIO_PIN_TX                     PICO_PINS_UART0_TX
#define RADIO_PIN_RX                     PICO_PINS_UART0_RX
#define RADIO_PIN_CTS                    PICO_PINS_UART0_CTS
#define RADIO_PIN_RTS                    PICO_PINS_UART0_RTS
#define RADIO_PIN_CONFIG                 (20)
#define RADIO_CONFIG_NON_VOLATILE_MEMORY (0)

#define UART_RADIO_ID             UART0_ID
#define UART_RADIO_BAUD_RATE      UART0_BAUD_RATE
// note : maybe configure flow control in radio NVM, before activating
#define UART_HW_FLOW_CONTROL_CTS UART0_CTS
#define UART_HW_FLOW_CONTROL_RTS UART0_RTS

#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define LOG_LEVEL_DEBUG (0)
#define PRINTF          (true)
// exit config state before entering new config state
#define RADIO_PRE_EXIT_CONFIG (1)

char payload_separator_char[1] = "-";

// TODO : remove / reduce
static void uart_wait(void) {
  sleep_ms(50); // todo : decrease
}

/**
 * @brief Enter the configuration state. Change from IDLE to CONFIG state.
 */
static void enter_config_state(void) {
  McuLog_trace("Enter config state");
#if LOG_LEVEL_DEBUG
  McuLog_debug("Config pin low");
#endif
  gpio_put(RADIO_PIN_CONFIG, false);
  sleep_us(50); // additional delay for gpio to be set
}

/**
 * @brief Exit the configuration state. Change from CONFIG to IDLE state.
 * fixme : maybe clear buffer after command
 */
void exit_config_state(void) {
  McuLog_trace("Exit config state");
#if LOG_LEVEL_DEBUG
  McuLog_debug("Config pin high");
  McuLog_debug("Send X to radio");
#endif
  gpio_put(RADIO_PIN_CONFIG, true);
  sleep_us(50); // additional delay for gpio to be set

  uart_puts(UART_RADIO_ID, "X");
  sleep_us(t_CONFIG_IDLE_US);
}

/**
 * @brief Initialize the radio module.
 */
void rc232_init() {
  // uart_init(UART_RADIO_ID, UART_RADIO_BAUD_RATE);

  /* UART configuration
   */
  uart_init(UART_RADIO_ID, 19200);
  /*
  uart_set_hw_flow(UART_RADIO_ID, UART_HW_FLOW_CONTROL_CTS,
                   UART_HW_FLOW_CONTROL_RTS);
                   */
  // todo : activate CTS and check (nvm already changed)
  // todo : activate RTS, configure nvm new and check
  uart_set_hw_flow(UART_RADIO_ID, UART_HW_FLOW_CONTROL_CTS,0 );

  uart_set_format(UART_RADIO_ID, DATA_BITS, STOP_BITS, PARITY);

  gpio_set_function(RADIO_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(RADIO_PIN_RX, GPIO_FUNC_UART);
#if UART_HW_FLOW_CONTROL_CTS
  gpio_set_function(RADIO_PIN_CTS, GPIO_FUNC_UART);
#endif
#if UART_HW_FLOW_CONTROL_RTS
  gpio_set_function(RADIO_PIN_RTS, GPIO_FUNC_UART);
#endif

  /* Pin configuration
   */
  // Enable VCC_RF
  gpio_init(PL_GPIO_ENABLE_VCC_RF);
  gpio_set_dir(PL_GPIO_ENABLE_VCC_RF, GPIO_OUT);
  gpio_put(PL_GPIO_ENABLE_VCC_RF, true);

  // Reset Pin
  gpio_init(PL_GPIO_RADIO_RESET);
  gpio_set_dir(PL_GPIO_RADIO_RESET, GPIO_OUT);
  gpio_put(PL_GPIO_RADIO_RESET, true);

  // Config Pin
  gpio_init(RADIO_PIN_CONFIG);
  gpio_set_dir(RADIO_PIN_CONFIG, GPIO_OUT);
  gpio_put(RADIO_PIN_CONFIG, true);

#if RADIO_CONFIG_NON_VOLATILE_MEMORY
  rc232_memory_configuration();
#endif
  /* Radio configuration (volatile memory)
   */
  // rc232_uart_read_all(); // clear buffer
  // rc232_config_rf_channel_number(1);
  // rc232_config_rf_power(1);
  // rc232_config_destination_address(20);
}

/**
 * @brief Reset the radio module via reset pin.
 *
 */
void rc232_reset(void) {
  McuLog_trace("Reset radio");
  gpio_put(PL_GPIO_RADIO_RESET, false);
  sleep_ms(100);
  gpio_put(PL_GPIO_RADIO_RESET, true);
}

/**
 * @brief Send a message to transmit.
 */
void rc232_tx_string(char *message) {
  if (!uart_is_writable(UART_RADIO_ID)) {
    McuLog_error("Radio UART not writable");
    return;
  }

  // RXD
  uart_puts(UART_RADIO_ID, message);

  // packet end character
  uart_puts(UART_RADIO_ID, "LF");
  sleep_us(100);
  // sleep packet timeout?

  if (UART_HW_FLOW_CONTROL_CTS) {
    sleep_us(t_RXD_CTS_US);
  } else {
    sleep_us(t_RXD_TX_US);
  }

  // time T_TX : depends on packet size and data rate, see formula datasheet
  sleep_ms(100);

  // sleep_us(t_TX_IDLE_US);
}

/**
 * @brief Readout all data from the radio buffer
 *
 */
void rc232_rx_read_buffer_full(void) {
  uint8_t rec_buffer[1];
  while (uart_is_readable(UART_RADIO_ID)) {
    uart_read_blocking(UART_RADIO_ID, rec_buffer, 1);
    McuLog_trace("Received %s from radio\n", rec_buffer);
    printf("Received %s from radio\n", rec_buffer);
  }
}

/**
 * @brief Read one byte from the radio buffer.
 *
 * @param buffer Buffer to store the received byte
 * @return error_t Error code
 */
error_t rc232_rx_read_byte(uint8_t *buffer) {
  if (!uart_is_readable(UART_RADIO_ID)) {
    return ERR_FAILED;
  }
  uart_read_blocking(UART_RADIO_ID, buffer, 1);

  return ERR_OK;
}

/**
 * @brief Set the destination address for transmission.
 *
 * @param address destination address
 * @note address length is dependent on addressing mode
 * @note volatile memory
 */
void rc232_config_destination_address(uint8_t address) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  enter_config_state();
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "T");
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Address
  uart_write_blocking(UART_RADIO_ID, &address, 1);
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  exit_config_state();
}

/**
 * @brief Set the RF channel number.
 *
 * @param channel RF channel number
 * @note volatile memory
 */
void rc232_config_rf_channel_number(uint8_t channel) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif
  enter_config_state();

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "C");
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Channel
  uart_write_blocking(UART_RADIO_ID, &channel, 1);

  sleep_us(t_CHANNEL_CONFIG_US);

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  exit_config_state();
}

/**
 * @brief Set the RF power.
 *
 * @param power RF power
 * @note volatile memory
 */
void rc232_config_rf_power(uint8_t power) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif
  enter_config_state();

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "P");
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Power
  uart_write_blocking(UART_RADIO_ID, &power, 1);

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  exit_config_state();
}

/**
 * @brief Read the temperature from the radio module.
 */
void rc232_read_temperature(void) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  enter_config_state();
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  // uart_write_blocking(UART_ID, &pre, 1);
  uart_puts(UART_RADIO_ID, "U");
  McuLog_trace("Send U to radio");
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  check_config_prompt(rec_buffer[1]);

  // Temperature calculation
  uint8_t temperature = rec_buffer[0] - 128;
  McuLog_trace("Temperature is : %d", temperature);
#if PRINTF
  printf("Temperature is : %d\n", temperature);
#endif

  exit_config_state();
}

/**
 * @brief Read the voltage from the radio module.
 */
void rc232_read_voltage(void) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  enter_config_state();
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  // uart_write_blocking(UART_ID, &pre, 1);
  uart_puts(UART_RADIO_ID, "V");
  McuLog_trace("Send U to radio");
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  check_config_prompt(rec_buffer[1]);

  // Voltage calculation
  uint8_t voltage = rec_buffer[0] * 0.030;
  McuLog_trace("Voltage is : %d", voltage);
#if PRINTF
  printf("Voltage is : %d\n", voltage);
#endif

  exit_config_state();
}

/**
 * @brief Read the signal strength from the radio module.
 *
 * @note Dependent on input signal strength P
 * @note P = - RSSI /2
 */
uint8_t rc232_signal_strength_indicator(void) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  enter_config_state();
  if (wait_config_prompt() == ERR_FAULT) {
    return 0; // fixme : error code, use pointer for value
  }

  // -- Send : Command byte
  // uart_write_blocking(UART_ID, &pre, 1);
  uart_puts(UART_RADIO_ID, "S");
  McuLog_trace("Send S to radio");
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  check_config_prompt(rec_buffer[1]);

  // -- Signal strenght calculation
  uint8_t rssi = rec_buffer[0];
  McuLog_trace("RSSI is : %d", rssi);
#if PRINTF
  printf("RSSI is : %d\n", rssi);
#endif

  exit_config_state();
}

/**
 * @brief Enters low current sleep mode for the radio module.
 *
 */
void rc232_sleep(void) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  enter_config_state();
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  gpio_put(RADIO_PIN_CONFIG, false);
  sleep_us(50); // additional delay for gpio to be set

  // -- Send : Command byte
  // uart_write_blocking(UART_ID, &pre, 1);
  uart_puts(UART_RADIO_ID, "Z");
  McuLog_trace("Send Z to radio");
  uart_wait();
  // don't exit config state. i.e hold gpio low.
}

/**
 * @brief Wakeup the radio from sleep mode.
 */
void rc232_wakeup(void) {
  gpio_put(RADIO_PIN_CONFIG, true);
  sleep_us(50); // additional delay for gpio to be set
}

uint8_t wait_config_prompt(void) {
  sleep_us(t_CONFIG_PROMPT_US);

  uint8_t rec_prompt[1];
  uart_read_blocking(UART_RADIO_ID, rec_prompt, 1);
  McuLog_trace("Received %d from radio\n", rec_prompt[0]);
  return check_config_prompt(rec_prompt[0]);
}

/**
 * @brief Get the configuration memory from the radio module.
 */
void rc232_get_configuration_memory(void) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  enter_config_state();
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "0");
  McuLog_trace("Send 0 to radio");
  uart_wait();

  // Readout buffer
  // fixme : values encoding in terminal
  rc232_rx_read_buffer_full();

  exit_config_state();
}

/**
 * @brief Check if the value received is equal to the prompt character '>'.
 *
 * @param received Value to check
 * @return uint8_t Error code
 */
uint8_t check_config_prompt(uint8_t received) {
  if (received != 62) {
    McuLog_error("Haven't received '>'");
    return ERR_FAULT;
  } else {
    received = 0;
    return ERR_OK;
  }
}

/**
 * @brief Read one byte from the non-volatile memory (NVM).
 *
 * @param address Data address in non-volatile memory (NVM)
 */
void rc232_memory_read_one_byte(uint8_t address) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif
  rc232_rx_read_buffer_full(); // fix : clear buffer, otherwise missing/wrong values

  enter_config_state();

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "Y");
  // McuLog_trace("Send Y to radio");
  McuLog_trace("Get NVM byte from radio");
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Parameters
  uart_write_blocking(UART_RADIO_ID, &address, 1);
#if PRINTF
  printf("address: 0x%02hhX (%d)\n", address, address);
#endif
  McuLog_trace("address: %d", address);
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
/* debug : print received buffer
for (uint8_t i = 0; i < buffer_size; i++) {
  McuLog_trace("Radio received [%d] : %d \n", i, rec_buffer[i]);
}
*/
#if PRINTF
  printf("config value : %d \n", rec_buffer[0]);
#endif
  McuLog_trace("config value : %d \n", rec_buffer[0]);

  exit_config_state();
}

/**
 * @brief Read the configurations from the non-volatile memory (NVM).
 *
 */
void rc232_memory_read_configuration(void) {
  rc232_memory_read_one_byte(NVM_ADDR_RF_CHANNEL);
  rc232_memory_read_one_byte(NVM_ADDR_RF_POWER);
  rc232_memory_read_one_byte(NVM_ADDR_RF_DATA_RATE);
  rc232_memory_read_one_byte(NVM_ADDR_PACKET_END_CHAR);
  rc232_memory_read_one_byte(NVM_ADDR_ADDRESS_MODE);
  rc232_memory_read_one_byte(NVM_ADDR_CRC);
  rc232_memory_read_one_byte(NVM_ADDR_UID);
  rc232_memory_read_one_byte(NVM_ADDR_SID);
  rc232_memory_read_one_byte(NVM_ADDR_DID);
  rc232_memory_read_one_byte(NVM_ADDR_UART_FW_CTRL);
  rc232_memory_read_one_byte(NVM_ADDR_LED_CONTROL);
}

/**
 * @brief Write into non-volatile memory (NVM) configuration.
 *
 */
void rc232_memory_write_configuration(void) {
#if RADIO_PRE_EXIT_CONFIG
  exit_config_state();
#endif

  /* IDLE -> CONFIG
   */
  enter_config_state();

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  /* CONFIG -> MEMORY CONFIG
   */
  // -- Send : Memory configuration state
  uart_puts(UART_RADIO_ID, "M");
  uart_wait();
  McuLog_trace("Send M to radio");
  McuLog_trace("Memory configuration state !");

  /* AVOID MULTIPLE WRITES <--
  // Configuration parameters {address, data}
  // -- RF Channel
  unsigned char config_channel[] = {NVM_ADDR_RF_CHANNEL, 5};
  uart_write_blocking(UART_RADIO_ID, config_channel, 2);
  uart_wait();
  McuLog_trace("Config NVM : RF Channel (Addr : %d, Value : %d)",
               config_channel[0], config_channel[1]);

  // -- RF Power
  unsigned char config_power[] = {NVM_ADDR_RF_POWER, 1};
  uart_write_blocking(UART_RADIO_ID, config_power, 2);
  uart_wait();
  McuLog_trace("Config NVM : RF Power (Addr : %d, Value : %d)", config_power[0],
               config_power[1]);

  // -- RF Data rate
  // 4 : 1.2kbit/s
  unsigned char config_data_rate[] = {NVM_ADDR_RF_DATA_RATE, 4};
  uart_write_blocking(UART_RADIO_ID, config_data_rate, 2);
  McuLog_trace("Config NVM : Data rate (Addr : %d, Value : %d)",
               config_data_rate[0], config_data_rate[1]);
  uart_wait();

  // -- Led Control
  unsigned char config_led[] = {NVM_ADDR_LED_CONTROL, 1};
  uart_write_blocking(UART_RADIO_ID, config_led, 2);
  uart_wait();
  McuLog_trace("Config NVM : Led Control (Addr : %d, Value : %d)",
               config_led[0], config_led[1]);

  // -- Unique ID
  unsigned char config_unique_id[] = {NVM_ADDR_UID, 6};
  uart_write_blocking(UART_RADIO_ID, config_unique_id, 2);
  uart_wait();
  McuLog_trace("Config NVM : Unique ID (UID) (Addr : %d, Value : %d)",
               config_unique_id[0], config_unique_id[1]);

  // -- System ID
  unsigned char config_system_id[] = {NVM_ADDR_SID, 1};
  uart_write_blocking(UART_RADIO_ID, config_system_id, 2);
  uart_wait();
  McuLog_trace("Config NVM : System ID (SID) (Addr : %d, Value : %d)",
               config_system_id[0], config_system_id[1]);

  // -- Destination ID
  unsigned char config_destination_id[] = {NVM_ADDR_DID, 20};
  uart_write_blocking(UART_RADIO_ID, config_destination_id, 2);
  uart_wait();
  McuLog_trace("Config NVM : Destination ID (DID) (Addr : %d, Value : %d)",
               config_destination_id[0], config_destination_id[1]);

  // -- Packet end character
  // 00 : NONE
  // 10 : LF
  unsigned char config_packet_end_char[] = {NVM_ADDR_PACKET_END_CHAR, 0x00};
  uart_write_blocking(UART_RADIO_ID, config_packet_end_char, 2);
  uart_wait();
  McuLog_trace("Config NVM : Packet end character (Addr : %d, Value : %d)",
               config_packet_end_char[0], config_packet_end_char[1]);

  // -- Address mode
  unsigned char config_address_mode[] = {NVM_ADDR_ADDRESS_MODE, 0x02};
  uart_write_blocking(UART_RADIO_ID, config_address_mode, 2);
  uart_wait();
  McuLog_trace("Config NVM : Packet end character (Addr : %d, Value : %d)",
               config_address_mode[0], config_address_mode[1]);

  // -- CRC mode
  unsigned char config_crc[] = {NVM_ADDR_CRC, 0x02};
  uart_write_blocking(UART_RADIO_ID, config_crc, 2);
  uart_wait();
  McuLog_trace("Config NVM : CRC mode (Addr : %d, Value : %d)", config_crc[0],
               config_crc[1]);

  // -- UART
  // 0 : None
  // 1 : CTS only
  // 3 : CTS/RTS only
  // 4 : RXTS (RS485)
  unsigned char config_uart_flow[] = {NVM_ADDR_UART_FW_CTRL, 0x03};
  uart_write_blocking(UART_RADIO_ID, config_uart_flow, 2);
  uart_wait();
  McuLog_trace("Config NVM : UART HW flow control (Addr : %d, Value : %d)",
               config_uart_flow[0], config_uart_flow[1]);
  */ // --> AVOID MULIPLE WRITES

  // todo : de-, enryption with key, vector (later)

  // -- Send : Exit
  unsigned char cmdExit[] = {NVM_CMD_EXIT};
  uart_write_blocking(UART_RADIO_ID, cmdExit, 1);
  // uart_write_blocking(UART_RADIO_ID, data1, sizeof(data1));
  McuLog_trace("Send %d to radio", cmdExit[0]);

  sleep_ms(t_MEMORY_CONFIG_MS);

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // fixme : clean buffer, contains '>' and values
  rc232_rx_read_buffer_full();

  exit_config_state();
  McuLog_trace("Exit memory configuration state !");
}
