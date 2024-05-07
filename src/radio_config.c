#include "radio_config.h"
#include "stdio.h"

#include "pico/stdlib.h"
#include "pico_config.h"

#include "McuLog.h"
#include "McuUtility.h"
#include "McuWait.h"

#include "hardware/uart.h"
#include <stdint.h>

#define RADIO_PIN_TX PICO_PINS_UART0_TX
#define RADIO_PIN_RX PICO_PINS_UART0_RX
#define RADIO_PIN_CONFIG (20)
#define RADIO_HW_FLOW_CONTROL (0)

#define UART_RADIO_ID UART0_ID
#define UART_RADIO_BAUD_RATE UART0_BAUD_RATE
#define UART_RADIO_CTS UART0_CTS
#define UART_RADIO_RTS UART0_RTS

#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

#define LOG_LEVEL_DEBUG (0)
#define PRINTF (1)

char payload_separator_char[1] = "-";

// TODO : remove / reduce
static void uart_wait(void) {
  sleep_ms(50); // todo : decrease
}

static void send_payload_separator(void) {
  uart_puts(UART_RADIO_ID, payload_separator_char);
  uart_wait();
}

/* Change from IDLE to CONFIG state
*/
static void enter_config_state(void) {
  McuLog_trace("Enter config state");
#if LOG_LEVEL_DEBUG
  McuLog_debug("Config pin low");
#endif
  gpio_put(RADIO_PIN_CONFIG, false);
  sleep_us(50); // additional delay for gpio to be set
}

/* Change from CONFIG to IDLE state
*/
static void exit_config_state(void) {
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

void radio_init() {
  // uart_init(UART_RADIO_ID, UART_RADIO_BAUD_RATE);
  uart_init(UART_RADIO_ID, 19200);
  gpio_set_function(RADIO_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(RADIO_PIN_RX, GPIO_FUNC_UART);

  // Enable VCC_RF
  gpio_init(PL_GPIO_ENABLE_VCC_RF);
  gpio_set_dir(PL_GPIO_ENABLE_VCC_RF, GPIO_OUT);
  gpio_put(PL_GPIO_ENABLE_VCC_RF, true);

  // Reset Pin
  gpio_init(PL_GPIO_RADIO_RESET);
  gpio_set_dir(PL_GPIO_RADIO_RESET, GPIO_OUT);
  gpio_put(PL_GPIO_RADIO_RESET, true);

  // HW flow control (default)
#if RADIO_HW_FLOW_CONTROL
  uart_set_hw_flow(UART_RADIO_ID, true, true);
#else
  uart_set_hw_flow(UART_RADIO_ID, false, false);
#endif

  uart_set_format(UART_RADIO_ID, DATA_BITS, STOP_BITS, PARITY);

  gpio_init(RADIO_PIN_CONFIG);
  gpio_set_dir(RADIO_PIN_CONFIG, GPIO_OUT);
  gpio_put(RADIO_PIN_CONFIG, true);
}

void radio_reset(void) {
  McuLog_trace("Reset radio");
  gpio_put(PL_GPIO_RADIO_RESET, false);
  sleep_ms(100);
  gpio_put(PL_GPIO_RADIO_RESET, true);
}

void radio_send(void) {
  if (!uart_is_writable(UART_RADIO_ID)) {
    McuLog_error("Radio UART not writable");
    return;
  }
  /* RXD
  */
  // send characters on UART line
  for (uint8_t i = 0; i < 100; i++) {
    uart_putc_raw(UART_RADIO_ID, 'H');
    sleep_us(100);
  }
  // sleep packet timeout?

  if (UART_RADIO_CTS) {
    sleep_us(t_RXD_CTS_US);
  } else {
    sleep_us(t_RXD_TX_US);
  }

  sleep_ms(100); // T_TX : depends on packet size and data rate, see formula datasheet
  sleep_us(t_TX_IDLE_US);

  // uart_putc_raw(UART_RADIO_ID, 21);
  // uart_wait();
  // send_payload_separator();
  // // printf("send: %s \r\n", strId);
  // uart_putc_raw(UART_RADIO_ID, 5);


}

void radio_uart_read_all(void) {
  uint8_t rec_buffer[1];
  while (uart_is_readable(UART_RADIO_ID)) {
    uart_read_blocking(UART_RADIO_ID, rec_buffer, 1);
    McuLog_trace("Received %s from radio\n", rec_buffer);
    printf("Received %s from radio\n", rec_buffer);
  }
}

void radio_memory_read_one_byte(uint8_t address) {
  

  // be sure to not be already in config state
  exit_config_state();

  enter_config_state();

  // -- Wait for '>'
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "Y");
  McuLog_trace("Send Y to radio");
  //uart_wait();

  // -- Wait for '>'
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Parameters
  uart_write_blocking(UART_RADIO_ID, &address, 1);
  McuLog_trace("parameter: %d \n", address);
  uart_wait();
  // todo : sleep

  // -- Receive : 1byte value + Prompt ('>')
  uint8_t buffer_size = 2;
  uint8_t rec_buffer[buffer_size];
  uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
  for (uint8_t i = 0; i < buffer_size; i++) {
    McuLog_trace("Radio received [%d] : %d \n", i, rec_buffer[i]);
  }

  exit_config_state();

  McuLog_trace("Finished memory read");
}

void radio_memory_configuration(void) {
  // be sure to not be already in config state
  exit_config_state();

  /* IDLE -> CONFIG
  */
  enter_config_state();

  // -- Wait for '>'
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  /* CONFIG -> MEMORY CONFIG
  */
  // -- Send : Memory configuration state
  uart_puts(UART_RADIO_ID, "M");
  McuLog_trace("Send M to radio");
  McuLog_trace("Memory configuration state !");
  uart_wait();

  // Configuration parameters {address, data}
  // -- Channel
  unsigned char config0[] = {0x00, 5};
  uart_write_blocking(UART_RADIO_ID, config0, 2);
  McuLog_trace("Send %d to radio", config0[0]);
  McuLog_trace("Send %d to radio", config0[1]);
  uart_wait();

  // -- Power
  unsigned char config1[] = {ADDR_RF_POWER, 1};
  uart_write_blocking(UART_RADIO_ID, config1, 2);
  McuLog_trace("Send %d to radio", config1[0]);
  McuLog_trace("Send %d to radio", config1[1]);
  uart_wait();

  // -- Data rate
  // 4 : 1.2kbit/s
  unsigned char config2[] = {ADDR_RF_DATA_RATE, 4};
  uart_write_blocking(UART_RADIO_ID, config2, 2);
  McuLog_trace("Send %d to radio", config2[0]);
  McuLog_trace("Send %d to radio", config2[1]);
  uart_wait();

  // todo : packet end character
  // todo : addressing (later)
  // todo : crc mode (later)
  // todo : uart flow control (later)
  // todo : led control (optional)
  // todo : de-, enryption with key, vector (later)

  // -- Send : Exit
  unsigned char cmdExit[] = {CMD_NVM_EXIT};
  uart_write_blocking(UART_RADIO_ID, cmdExit, 1);
  // uart_write_blocking(UART_RADIO_ID, data1, sizeof(data1));
  McuLog_trace("Send %d to radio", cmdExit[0]);
  sleep_ms(t_MEMORY_CONFIG_MS);

  // -- Wait for '>'
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // fixme : '>' and values in buffer
  radio_uart_read_all();

  exit_config_state();
  McuLog_trace("Exit memory configuration state !");
}

/* Destination address
 * @param address : destination address (length dependent on addressing mode)
*/
void radio_destination_address(uint8_t address) {
  // be sure to not be already in config state
  exit_config_state();

  enter_config_state();
  // -- Wait for '>'
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
}

void radio_read_temperature(void) {
  uint8_t rec_prompt[1];
  bool continueConfig = false;

  // be sure to not be already in config state
  exit_config_state();

  enter_config_state();
  // -- Wait for '>'
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
  McuLog_trace("Finished read temperature");
}

uint8_t wait_config_prompt(void) {
  sleep_us(t_CONFIG_PROMPT_US);

  uint8_t rec_prompt[1];
  uart_read_blocking(UART_RADIO_ID, rec_prompt, 1);
  McuLog_trace("Received %d from radio\n", rec_prompt[0]);
  return check_config_prompt(rec_prompt[0]);
}

void radio_get_configuration_memory(void){
  // be sure to not be already in config state
  exit_config_state();

  enter_config_state();
  // -- Wait for '>'
  if (wait_config_prompt() == ERR_FAULT) {
    return;
  }

  // -- Send : Command byte
  uart_puts(UART_RADIO_ID, "0");
  McuLog_trace("Send 0 to radio");
  uart_wait();

  // Readout buffer
  // fixme : values encoding in terminal
  radio_uart_read_all();
}

uint8_t check_config_prompt(uint8_t received) {
  if (received != 62) {
    McuLog_error("Haven't received '>'");
    return ERR_FAULT;
  } else {
    received = 0;
    return ERR_OK;
  }
}
