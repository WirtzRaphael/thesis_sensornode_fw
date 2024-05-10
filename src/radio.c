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
#include <string.h>
#include <sys/_types.h>

#define RF_CHANNEL_DEFAULT (5)
#define PROTOCOL_AUTH_SIZE_BYTES (10)

// Use fix channel or scan channels
#define FEAT_SCAN_CHANNELS (0)

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
 * todo : fix channel or channel scan feature (time)
 * todo : use broadcast for tranmission (coverage vs security)
 */
void radio_authentication(void) {
  rc232_uart_read_all(); // empty buffer
  McuLog_trace("radio : Scan network and authenticate\n");

#if FEAT_SCAN_CHANNELS // scan channels
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
    // todo : implement receive response and decode
    //uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
    uint8_t buffer[10];
    for (int t = 0; t <= 300; t++) {
      rc232_uart_read_byte(buffer);
      if(buffer[0] == '0') {
        // continue until protcol start character (A,A,C)
        continue;
      }
      // todo : check for sequence 
      McuLog_trace("radio : received %c\n", buffer[0]);
      /*
      if(radio_receive_authentication() == ERR_OK) {
        // error code
        McuLog_trace("radio : authentication successful\n");
        break;
      };
      */
      sleep_ms(10); // fixme : delay until sent
    }
    // todo wait response
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
  char frame_flag[1] = { 0x7E};
  rc232_send_string(frame_flag);
  // -- payload
  char message [3]; 
  // todo : combine arrays and send all together
  // - address
  // NONE (before authentication not assigned)
  // - control field
  // authentication request
  strcpy(message, "AR");
  // - board information
  strcat(message, pico_uid_string);
  // -- checksum 
  printf("payload (string): %s\n", message);
  // -- convert to binary
  // todo : convert to binary
  //unsigned char* message_byte = (unsigned char*) message;
  //printf("payload (binary): %s\n", message_byte);
  rc232_send_string(message);

  // -- frame delimiter flag
  //strcat(message, "0x7E"); // 
  rc232_send_string(frame_flag);
}

error_t radio_receive_authentication(void) {
  // todo : protocol authentication response
  // todo : receive -> rx (rc232)
  // receive via broadcast
  //uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
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
