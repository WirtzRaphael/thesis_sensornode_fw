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
 * todo : rtos delay instead sleep
 * todo : replace magic delays
 */
#include "rc232.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include "stdio.h"

#include "pico/stdlib.h"
#include "platform_config.h"

#include "McuLog.h"
#include "McuUtility.h"
#include "McuWait.h"

#include "hardware/uart.h"
#include <errno.h>
#include <stdint.h>

#if PLATFORM_CONFIG_USE_RADIO

  #define RADIO_PIN_TX     PICO_PINS_UART1_TX
  #define RADIO_PIN_RX     PICO_PINS_UART1_RX
  #define RADIO_PIN_CTS    PICO_PINS_UART1_CTS
  #define RADIO_PIN_RTS    PICO_PINS_UART1_RTS
  #define RADIO_PIN_CONFIG PL_GPIO_RADIO_CONFIG
  // todo [demo] : ?
  #define RADIO_CONFIG_NON_VOLATILE_MEMORY (0)

  // note : maybe configure flow control in radio NVM, before activating
  #define UART_RADIO_ID            UART1_ID
  #define UART_RADIO_BAUD_RATE     UART1_BAUD_RATE
  #define UART_HW_FLOW_CONTROL_CTS UART1_CTS
  #define UART_HW_FLOW_CONTROL_RTS UART1_RTS

  #define UART_RADIO_DATA_BITS 8
  #define UART_RADIO_STOP_BITS 1
  #define UART_RADIO_PARITY    UART_PARITY_NONE

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
  McuLog_trace("[rc232] Enter config state");
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
  McuLog_trace("[rc232] Exit config state");
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
  // todo : activate RTS, configure nvm new and check
  uart_init(UART_RADIO_ID, UART_RADIO_BAUD_RATE);
  // uart_set_hw_flow(UART_RADIO_ID, UART_HW_FLOW_CONTROL_CTS,
  //                    UART_HW_FLOW_CONTROL_RTS);
  uart_set_hw_flow(UART_RADIO_ID, UART_HW_FLOW_CONTROL_CTS, 0);

  uart_set_format(UART_RADIO_ID, UART_RADIO_DATA_BITS, UART_RADIO_STOP_BITS,
                  UART_RADIO_PARITY);

  gpio_set_function(RADIO_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(RADIO_PIN_RX, GPIO_FUNC_UART);
  #if UART_HW_FLOW_CONTROL_CTS
  gpio_set_function(RADIO_PIN_CTS, GPIO_FUNC_UART);
  #else
  gpio_set_function(RADIO_PIN_CTS, GPIO_IN);
  #endif
  #if UART_HW_FLOW_CONTROL_RTS
  gpio_set_function(RADIO_PIN_RTS, GPIO_FUNC_UART);
  #else
  gpio_set_function(RADIO_PIN_RTS, GPIO_IN);
  #endif

  // todo : move central
  /* Pin configuration
   */
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

void rc232_deinit(void) {
  // uart
  uart_deinit(UART_RADIO_ID);

  // gpio
  gpio_set_dir(PL_GPIO_RADIO_RESET, GPIO_IN);
  gpio_set_dir(PL_GPIO_RADIO_CONFIG, GPIO_IN);
  // gpio_deinit(RADIO_PIN_RX);
  gpio_set_dir(RADIO_PIN_RX, GPIO_IN);
  // gpio_deinit(RADIO_PIN_TX);
  gpio_set_dir(RADIO_PIN_TX, GPIO_IN);
}

/**
 * @brief Reset the radio module via reset pin.
 *
 */
void rc232_reset(void) {
  McuLog_trace("[rc232] Reset radio");
  gpio_put(PL_GPIO_RADIO_RESET, false);
  sleep_ms(100);
  gpio_put(PL_GPIO_RADIO_RESET, true);
}

/**
 * @brief Transmit string
 */
void rc232_tx_packet_string(const uint8_t *message, bool dryrun) {
  McuLog_trace("[rc232] TX data (string) \n");
  if (!uart_is_writable(UART_RADIO_ID)) {
    McuLog_error("Radio UART not writable");
    return;
  }

  // RXD
  McuLog_trace("[rc232] Send message : %s", message);
  if (!dryrun) {
    // todo : check implicit CR/LF conversion in uart_puts i.e. uart_putc
    uart_puts(UART_RADIO_ID, message);
    // No explicit packet end character
  }

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
 * @brief Transmit bytes
 * todo : change bytes to char?
 */
void rc232_tx_packet_bytes(uint8_t *bytes, size_t length, bool dryrun) {
  McuLog_trace("[rc232] TX data (bytes) \n");
  if (!uart_is_writable(UART_RADIO_ID)) {
    McuLog_error("Radio UART not writable");
    return;
  }

  // RXDprint_binary
  for (uint8_t i = 0; i < length; i++) {
    McuLog_trace("[rc232] Send byte : %u \n", bytes[i]);
  }
  if (!dryrun) {
    uart_write_blocking(UART_RADIO_ID, bytes, length);
    // No explicit packet end character
  }

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

  sleep_us(100);
}

/**
 * @brief Readout all data from the radio buffer
 *
 * todo : function to only flush/clear buffer (?)
 */
void rc232_rx_read_buffer_full(void) {
  uint8_t rec_buffer[1];
  while (uart_is_readable(UART_RADIO_ID)) {
    uart_read_blocking(UART_RADIO_ID, rec_buffer, 1);
    McuLog_trace("[rc232] Received %s from radio\n", rec_buffer);
    printf("Received %s from radio\n", rec_buffer);
    printf("Received %d from radio\n", *rec_buffer);
  }
}

/**
 * @brief Read one byte from the radio buffer non-blocking guard.
 *
 * @param buffer Buffer to store the received byte
 * @return error_t Error code
 */
error_t rc232_rx_read_bytes(uint8_t *buffer, size_t length) {
  if (!uart_is_readable(UART_RADIO_ID)) {
    return ERR_FAILED;
  }
  uart_read_blocking(UART_RADIO_ID, buffer, length);

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
  McuLog_trace("[rc232] Send U to radio");
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("[rc232] Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  check_config_prompt(rec_buffer[1]);

  // Temperature calculation
  uint8_t temperature = rec_buffer[0] - 128;
  McuLog_trace("[rc232] Temperature is : %d", temperature);
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
  McuLog_trace("[rc232] Send U to radio");
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("[rc232] Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  check_config_prompt(rec_buffer[1]);

  // Voltage calculation
  uint8_t voltage = rec_buffer[0] * 0.030;
  McuLog_trace("[rc232] Voltage is : %d", voltage);
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
  McuLog_trace("[rc232] Send S to radio");
  uart_wait();

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("[rc232] Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  check_config_prompt(rec_buffer[1]);

  // -- Signal strenght calculation
  uint8_t rssi = rec_buffer[0];
  McuLog_trace("[rc232] RSSI is : %d", rssi);
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
  McuLog_trace("[rc232] Send Z to radio");
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
  McuLog_trace("[rc232] Received %d from radio\n", rec_prompt[0]);
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
  McuLog_trace("[rc232] Send 0 to radio");
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
  rc232_rx_read_buffer_full(); // fix : clear buffer, otherwise missing/wrong
                               // values

  enter_config_state();

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "Y");
  // McuLog_trace("[rc232] Send Y to radio");
  McuLog_trace("[rc232] Get NVM byte from radio");
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Parameters
  uart_write_blocking(UART_RADIO_ID, &address, 1);
  #if PRINTF
  printf("address: 0x%02hhX (%d)\n", address, address);
  #endif
  McuLog_trace("[rc232] address: %d", address);
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
  McuLog_trace("[rc232] config value : %d \n", rec_buffer[0]);

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
  McuLog_trace("[rc232] Send M to radio");
  McuLog_trace("[rc232] Memory configuration state !");

  /* AVOID MULTIPLE WRITES <--
  // Configuration parameters {address, data}
  // -- RF Channel
  unsigned char config_channel[] = {NVM_ADDR_RF_CHANNEL, 5};
  uart_write_blocking(UART_RADIO_ID, config_channel, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : RF Channel (Addr : %d, Value : %d)",
               config_channel[0], config_channel[1]);

  // -- RF Power
  unsigned char config_power[] = {NVM_ADDR_RF_POWER, 1};
  uart_write_blocking(UART_RADIO_ID, config_power, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : RF Power (Addr : %d, Value : %d)",
  config_power[0], config_power[1]);

  // -- RF Data rate
  // 4 : 1.2kbit/s
  unsigned char config_data_rate[] = {NVM_ADDR_RF_DATA_RATE, 4};
  uart_write_blocking(UART_RADIO_ID, config_data_rate, 2);
  McuLog_trace("[rc232] Config NVM : Data rate (Addr : %d, Value : %d)",
               config_data_rate[0], config_data_rate[1]);
  uart_wait();

  // -- Led Control
  unsigned char config_led[] = {NVM_ADDR_LED_CONTROL, 1};
  uart_write_blocking(UART_RADIO_ID, config_led, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : Led Control (Addr : %d, Value : %d)",
               config_led[0], config_led[1]);

  // -- Unique ID
  unsigned char config_unique_id[] = {NVM_ADDR_UID, 6};
  uart_write_blocking(UART_RADIO_ID, config_unique_id, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : Unique ID (UID) (Addr : %d, Value : %d)",
               config_unique_id[0], config_unique_id[1]);

  // -- System ID
  unsigned char config_system_id[] = {NVM_ADDR_SID, 1};
  uart_write_blocking(UART_RADIO_ID, config_system_id, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : System ID (SID) (Addr : %d, Value : %d)",
               config_system_id[0], config_system_id[1]);

  // -- Destination ID
  unsigned char config_destination_id[] = {NVM_ADDR_DID, 20};
  uart_write_blocking(UART_RADIO_ID, config_destination_id, 2);
  uart_wait();
  McuLog_trace(
      "[rc232] Config NVM : Destination ID (DID) (Addr : %d, Value : %d)",
      config_destination_id[0], config_destination_id[1]);

  // -- Packet end character
  // 00 : NONE
  // 10 : LF
  unsigned char config_packet_end_char[] = {NVM_ADDR_PACKET_END_CHAR, 0x00};
  uart_write_blocking(UART_RADIO_ID, config_packet_end_char, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : Packet end character (Addr : %d, Value :
  %d)", config_packet_end_char[0], config_packet_end_char[1]);

  // -- Address mode
  unsigned char config_address_mode[] = {NVM_ADDR_ADDRESS_MODE, 0x02};
  uart_write_blocking(UART_RADIO_ID, config_address_mode, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : Packet end character (Addr : %d, Value :
  %d)", config_address_mode[0], config_address_mode[1]);

*/
  // -- CRC mode
  unsigned char config_crc[] = {NVM_ADDR_CRC, 0x02};
  uart_write_blocking(UART_RADIO_ID, config_crc, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : CRC mode (Addr : %d, Value : %d)",
  config_crc[0], config_crc[1]);
  /*

  // -- UART
  // 0 : None
  // 1 : CTS only
  // 3 : CTS/RTS only
  // 4 : RXTS (RS485)
  // fixme : RTS blocking
  unsigned char config_uart_flow[] = {NVM_ADDR_UART_FW_CTRL, 0x01};
  uart_write_blocking(UART_RADIO_ID, config_uart_flow, 2);
  uart_wait();
  McuLog_trace("[rc232] Config NVM : UART HW flow control (Addr : %d, Value :
  %d)", config_uart_flow[0], config_uart_flow[1]);
  */ // --> AVOID MULIPLE WRITES

  // todo : de-, enryption with key, vector (later)

  // -- Send : Exit
  unsigned char cmdExit[] = {NVM_CMD_EXIT};
  uart_write_blocking(UART_RADIO_ID, cmdExit, 1);
  // uart_write_blocking(UART_RADIO_ID, data1, sizeof(data1));
  McuLog_trace("[rc232] Send %d to radio", cmdExit[0]);

  sleep_ms(t_MEMORY_CONFIG_MS);

  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // fixme : clean buffer, contains '>' and values
  rc232_rx_read_buffer_full();

  exit_config_state();
  McuLog_trace("[rc232] Exit memory configuration state !");
}

#endif /* CONFIG_USE_RADIO */