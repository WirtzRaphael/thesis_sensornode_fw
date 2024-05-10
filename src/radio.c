/**
 * @file radio.c
 * @author raphael wirtz
 * @brief Radio protocol
 * @version 1
 * @date 2024-05-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "radio.h"
#include "McuLog.h"
#include "McuUtility.h"
#include "pico/stdlib.h"
#include "rc232.h"

// #include "pico_config.h"

#include "pico/unique_id.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_types.h>

#define RF_CHANNEL_DEFAULT       (5)
#define PROTOCOL_AUTH_SIZE_BYTES (10)

// Use fix channel or scan channels
#define SCAN_CHANNELS_FOR_CONNECTION (0)
#define TRANSMISSION_IN_BYTES        (0)
#define ACTIVATE_RF                  (0)

int channel_start = 0;
int channel_end = 0;

pico_unique_board_id_t pico_uid = {0};
char pico_uid_string[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 +
                     1]; // Each byte to two hex chars + null terminator

// todo : sync time
// todo : buffer handling (?) -> rc232
// todo : temperature sensor values

void radio_init(void) {
  pico_get_unique_board_id_string(pico_uid_string, sizeof(pico_uid_string));
  McuLog_trace("pico unique id: %s \n", pico_uid_string);
}

/**
 * @brief Scan radio network and authenticate.
 *
 * todo : refactor when not direct radio buffer used
 */
void radio_authentication(void) {
  rc232_uart_read_all(); // empty buffer
  McuLog_trace("radio : Scan network and authenticate\n");

#if SCAN_CHANNELS_FOR_CONNECTION
  channel_start = RC1701_RF_CHANNEL_MIN;
  channel_end = RC1701_RF_CHANNEL_MAX;
#else // only default channel
  channel_start = RF_CHANNEL_DEFAULT;
  channel_end = RF_CHANNEL_DEFAULT;
#endif

  // check channel range
  if (!(channel_start >= 1 && channel_end <= 10)) {
    McuLog_error("radio : invalid channel range\n");
    return;
  }

  // send request to broadcast address
  rc232_config_destination_address(RC232_BROADCAST_ADDRESS);
  for (int i = channel_start; i <= channel_end; i++) {
    McuLog_trace("radio : scanning channel %d\n", i);

    rc232_config_rf_channel_number(i);
    radio_send_authentication_request();
    /*
     * Wait for response and read control character
     * todo : refactor, don't read byte by byte
     * todo : hscl lite protocol
     */
    // uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
    uint8_t buffer1[1];
    uint8_t buffer2[1];
    error_t err = ERR_ARBITR;
    for (int t = 0; t <= 500; t++) {
      err = rc232_uart_read_byte(buffer1);

      if (err == ERR_OK) {
        // first char
        McuLog_trace("radio : received %c\n", buffer1[0]);
        printf("radio : received %c\n", buffer1[0]);
        err = rc232_uart_read_byte(buffer2);
        if (err == ERR_OK) {
          // second char
          printf("radio : received %c\n", buffer2[0]);
          printf("radio: received %c %c\n", buffer1[0], buffer2[0]);
        }
      }
      if (err == ERR_OK) {
        sleep_ms(10); // fixme : delay until sent
      }
    }
    /*
     * Action based on received control character
     */
    if (buffer1[0] == 'A' && buffer2[0] == 'C') {
      McuLog_trace("radio : acknowledge received\n");
      // todo : action after acknowledge (payload values, set uid, channel)
      break;
    }
  }
}

rc232_config_destination_address(20);
rc232_config_rf_channel_number(5);
// receive response
// - Free UID network (optional)
// - UID gateway -> DID
// - time sync (optional)
}

void radio_send_authentication_request(void) {
  // todo : HDLC-Lite (FRAMING PROTOCOL, high-level data link control)
  // todo : protocol authentication request
  // todo : send -> tx (rc232)
  // -- frame delimiter flag
  // fixme : change buffer from char to int (binary)
  char frame_flag[5] = "0x7E";
  // -- payload
  char payload[3];
  // todo : combine arrays and send all together
  // - address
  // NONE (before authentication not assigned)
  // - control field
  // authentication request
  strcpy(payload, "AR");
  printf("payload (string): %s\n", payload);
  strcat(payload, "-");
  printf("payload (string): %s\n", payload);
  // - board information
  printf("pico uid string: %s\n", pico_uid_string);
  strcat(payload, pico_uid_string);
  printf("payload (string): %s\n", payload);
// -- checksum
#if TRANSMISSION_IN_BYTES
  McuLog_trace("convert message to binary");
// -- convert to binary
// todo : convert to binary and send binary
// todo : bit stuffing
// todo : escape character
// todo : crc (optional)
// unsigned char* message_byte = (unsigned char*) message;
// printf("payload (binary): %s\n", message_byte);
#else
  // -- send
  // todo : string stuffing
  // todo : escape character
  // todo : crc (optional)
  // frame delimiter flag
  if (ACTIVATE_RF) {
    rc232_send_string(frame_flag);
    // message
    rc232_send_string(payload);
    // frame delimiter flag
    rc232_send_string(frame_flag);
  }
#endif
}

error_t radio_receive_authentication(void) {
  // todo : protocol authentication response
  // todo : receive -> rx (rc232)
  // receive via broadcast
  // uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
  uint8_t buffer[10];
  if (rc232_uart_read_byte(buffer) != ERR_OK) {
    return ERR_FAILED;
  }
  if (buffer[0] != 'A') {
    return ERR_FAILED;
  }
  if (rc232_uart_read_byte(buffer) != ERR_OK) {
    return ERR_FAILED;
  }
  if (buffer[1] != 'A') {
    return ERR_FAILED;
  }
  if (rc232_uart_read_byte(buffer) != ERR_OK) {
    return ERR_FAILED;
  }
  if (buffer[2] != 'C') {
    return ERR_FAILED;
  }

  return ERR_OK;
}

void radio_send_temperature(void) {

  rc232_send_string("temperature values");
  // todo : protocol temperature / sensor values
}

void radio_send_test(void) { rc232_send_string("hello world"); }

// pico_get_unique_board_id(&pico_unique_board_id);

/**
 * @brief Send payload separator character.
 */
static void send_payload_separator(void) {
  // todo
}
