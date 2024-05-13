/**
 * @file radio.c
 * @author raphael wirtz
 * @brief Radio protocol
 * @date 2024-05-09
 *
 * @copyright Copyright (c) 2024
 *
 * todo : sync time
 * todo : buffer handling (?) -> rc232
 */
#include "radio.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "McuUtility.h"
#include "cobs.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/unique_id.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_types.h>
//
#include "application.h"
#include "rc232.h"
#include "sensors.h"

#define RF_CHANNEL_DEFAULT          (5)
#define RF_CHANNEL_MIN              (1)
#define RF_CHANNEL_MAX              (10)
#define RF_DESTINATION_ADDR_DEFAULT (20)
#define PROTOCOL_AUTH_SIZE_BYTES    (10)

// Use fix channel or scan channels
#define SCAN_CHANNELS_FOR_CONNECTION (0)
#define TRANSMISSION_IN_BYTES        (0)
#define ACTIVATE_RF                  (0)
#define PRINTF_RF                    (0)

#ifndef dimof
  #define dimof(X) (sizeof(X) / sizeof((X)[0]))
#endif

int rf_channel_start = 0;
int rf_channel_end = 0;
char rf_channel = RF_CHANNEL_DEFAULT;
char rf_destination_address = RF_DESTINATION_ADDR_DEFAULT;

static uint16_t radio_time_intervals_ms = 500;

typedef struct {
  const uint8_t *data_ptr;
  size_t data_len;
  const uint8_t *encoded_ptr;
  size_t encoded_len;
  const char *description_ptr;
} data_info;

static const data_info cobs_tests[] = {
    {"", 0, "\x01", 1, "Empty"},
    {"1", 1,
     "\x02"
     "1",
     2, "1 non-zero byte"},
    {"12345", 5,
     "\x06"
     "12345",
     6, "5 non-zero bytes"},
    {"12345\x00"
     "6789",
     10,
     "\x06"
     "12345\x05"
     "6789",
     11, "Zero in middle"},
    {"\x00"
     "12345\x00"
     "6789",
     11,
     "\x01\x06"
     "12345\x05"
     "6789",
     12, "Zero at start and middle"},
    {"12345\x00"
     "6789\x00",
     11,
     "\x06"
     "12345\x05"
     "6789\x01",
     12, "Zero at start and end"},
    {"\x00", 1, "\x01\x01", 2, "1 zero byte"},
    {"\x00\x00", 2, "\x01\x01\x01", 3, "2 zero bytes"},
    {"\x00\x00\x00", 3, "\x01\x01\x01\x01", 4, "3 zero bytes"},
};

pico_unique_board_id_t pico_uid = {0};
char pico_uid_string[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 +
                     1]; // Each byte to two hex chars + null terminator

static void vRadioTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xDelay_radio_task = pdMS_TO_TICKS(radio_time_intervals_ms);
  const TickType_t xButtonSemaphore = portTICK_PERIOD_MS * 50;

  for (;;) {
    // periodic task
    vTaskDelayUntil(&xLastWakeTime, xDelay_radio_task);
    // sensors_print_temperature_xQueue_latest(xQueue_temperature_sensor_1);
    // sensors_print_temperature_xQueue_latest(xQueue_temperature_sensor_2);
    // todo : readout buffer and decompose
    // todo : receive fw file (download) (block resource, write to flash, signal
    // for update)

    if (xSemaphoreTake(xButtonASemaphore, xButtonSemaphore) == pdTRUE) {
      printf("[radio] Semaphore take Button A\n");
      printf("[radio] Synchronize / Authentication");
      // todo : authentication, connection gateway (sync time)
    }
    if (xSemaphoreTake(xButtonBSemaphore, xButtonSemaphore) == pdTRUE) {
      printf("[radio] Semaphore take Button B\n");
      printf("[radio] Send sensors values \n");
      // todo : readout all values queue
      // todo : case empty queue / no new value / nothing to send
      temperature_measurement_t temperature_measurement_sensor1 = {0, 0, 0};
      sensors_temperature_xQueue_receive(xQueue_temperature_sensor_1,
                                         &temperature_measurement_sensor1);
      radio_send_temperature_as_string(&temperature_measurement_sensor1, true);
      radio_send_temperature_as_bytes(&temperature_measurement_sensor1, false);
      // todo : send sensor 2
    }

    //  printf("radio killed the video star.");
  }
}

/**
 * @brief Initialize radio.
 */
void radio_init(void) {
  pico_get_unique_board_id_string(pico_uid_string, sizeof(pico_uid_string));
  McuLog_trace("pico unique id: %s \n", pico_uid_string);

  // BaseType_t xReturned;
  // TaskHandle_t xHandle = NULL;
  if (xTaskCreate(vRadioTask, /* pointer to the task */
                  "radio",    /* task name for kernel awareness debugging */
                  1000 / sizeof(StackType_t), /* task stack size */
                  (void *)NULL,         /* optional task startup argument */
                  tskIDLE_PRIORITY + 2, /* initial priority */
                  (TaskHandle_t *)NULL  /* optional task handle to create */
                  ) != pdPASS) {
    for (;;) {
    } /* error! probably out of memory */
  }
}

char radio_get_rf_destination_address(void) { return rf_destination_address; }

/**
 * @brief Scan radio network and authenticate.
 *
 * - Scans channels (optional)
 * - Send authentication request
 * - Waiting for acknowledge
 * -
 * todo : save or directly use response, like:
 * - Free UID network (optional)
 * - UID gateway -> DID
 * - time sync (optional)
 * todo : refactor when not direct radio buffer used i.e. radio task
 */
void radio_authentication(void) {
  rc232_rx_read_buffer_full(); // empty buffer
  McuLog_trace("radio : Scan network and authenticate\n");

#if SCAN_CHANNELS_FOR_CONNECTION
  channel_start = RC1701_RF_CHANNEL_MIN;
  channel_end = RC1701_RF_CHANNEL_MAX;
#else // only default channel
  rf_channel_start = RF_CHANNEL_DEFAULT;
  rf_channel_end = RF_CHANNEL_DEFAULT;
#endif

  // check channel range
  if (!(rf_channel_start >= 1 && rf_channel_end <= 10)) {
    McuLog_error("radio : invalid channel range\n");
    return;
  }

  // send request to broadcast address
  rc232_config_destination_address(RC232_BROADCAST_ADDRESS);
  for (int i = rf_channel_start; i <= rf_channel_end; i++) {
    McuLog_trace("radio : scanning channel %d\n", i);

    rc232_config_rf_channel_number(i);
    radio_send_authentication_request();
    /*
     * Wait for response and read control character
     * todo : refactor, don't read byte by byte
     */
    // uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
    uint8_t buffer1[1];
    uint8_t buffer2[1];
    error_t err = ERR_ARBITR;
    for (int t = 0; t <= 500; t++) {
      err = rc232_rx_read_byte(buffer1);

      if (err == ERR_OK) {
        // first char
        McuLog_trace("radio : received %c\n", buffer1[0]);
        printf("radio : received %c\n", buffer1[0]);
        err = rc232_rx_read_byte(buffer2);
        if (err == ERR_OK) {
          // second char
          printf("radio : received %c\n", buffer2[0]);
          McuLog_trace("radio : received %c\n", buffer2[0]);
          printf("radio: received %c %c\n", buffer1[0], buffer2[0]);
          /*
           * Action based on received control character
           */
          if (buffer1[0] == 'A' && buffer2[0] == 'C') {
            McuLog_trace("radio : acknowledge received\n");
            // todo : action after acknowledge (payload values, set uid,
            // channel) receive response
            // - Free UID network (optional)
            // - UID gateway -> DID
            // - time sync (optional)
            break;
          }
        }
      }
      if (err == ERR_OK) {
        sleep_ms(10); // fixme : delay until sent
      }
    }
  }

  // reset to default address
  rc232_config_destination_address(rf_destination_address);
  rc232_config_rf_channel_number(rf_channel);
}

/**
 * @brief Send authentication request.
 */
static void radio_send_authentication_request(void) {
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
  // todo : binary command instruction
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
  rc232_tx_packet_string(frame_flag, ACTIVATE_RF);
  // message
  rc232_tx_packet_string(payload, ACTIVATE_RF);
  // frame delimiter flag
  rc232_tx_packet_string(frame_flag, ACTIVATE_RF);
#endif
}

/**
 * @brief Wait for authentication response.
 *
 * @return error_t
 * todo : refactor, don't read byte by byte
 * todo : hscl lite protocol
 */
static error_t radio_wait_for_authentication_response(uint32_t timeout_ms) {
  // uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
  uint8_t buffer1[1];
  uint8_t buffer2[1];
  error_t err = ERR_ARBITR;

  // wait until response or timeout
  for (int t = 0; t <= timeout_ms; t++) {
    // read buffer - one char
    err = rc232_rx_read_byte(buffer1);

    // check if received
    if (err == ERR_OK) {
      // first char
      McuLog_trace("radio : received %c\n", buffer1[0]);
      printf("radio : received %c\n", buffer1[0]);
      // read buffer - one char
      err = rc232_rx_read_byte(buffer2);
      // check if received
      if (err == ERR_OK) {
        // second char
        printf("radio : received %c\n", buffer2[0]);
        McuLog_trace("radio : received %c\n", buffer2[0]);
        printf("radio: received %c %c\n", buffer1[0], buffer2[0]);
        /*
         * Action based on received control character
         */
        if (buffer1[0] == 'A' && buffer2[0] == 'C') {
          McuLog_trace("radio : acknowledge received\n");
          // todo : action after acknowledge (payload values, set uid, etc.)
          // channel) receive response
          // - Free UID network (optional)
          // - UID gateway -> DID
          // - time sync (optional)
          break;
        }
      }
    }
    sleep_ms(1);
  }
  return ERR_OK;
}

/**
 * @brief Send temperature values.
 *
 */
void radio_send_temperature_as_string(
    temperature_measurement_t *temperature_measurement, bool dryrun) {
  // Payload content
  uint8_t buffer[32];
  char *separator = "-";
  McuUtility_NumFloatToStr(buffer, sizeof(buffer),
                           temperature_measurement->temperature, 2);
  McuUtility_strcat(buffer, sizeof(buffer), separator);
  McuUtility_strcatNum8u(buffer, sizeof(buffer), temperature_measurement->id);
  McuUtility_strcat(buffer, sizeof(buffer), separator);
  McuUtility_strcatNum16u(buffer, sizeof(buffer),
                          temperature_measurement->timediff_to_start);
#if PRINTF_RF
  printf("send temperature: %f\n", (temperature_measurement->temperature));
  printf("send temperature (buffer): %s\n", buffer);
#endif
  printf("send temperature (buffer): %s\n", buffer);

  // Send
  rc232_tx_packet_string(buffer, dryrun);
}

/**
 * @brief Send temperature values as bytes.
 *
 * todo : refactor
 * todo : time information
 * todo : compress format / algorithm to reduce number of temperatures (eg only
 * todo : hdlc-lite / cobs
 * diffs) todo : error code return
 */
void radio_send_temperature_as_bytes(
    temperature_measurement_t *temperature_measurement, bool dryrun) {
  // uint8_t hdlc_frame_flag = 0x7E;
  uint8_t payload_byte[530] = {0};
  // -- content descirption field
  // -- encode
  uint8_t encode_out[COBS_ENCODE_DST_BUF_LEN_MAX(530)];
  cobs_encode_result encoded;
  size_t i;
  for (i = 0; i < dimof(cobs_tests); i++) {
    memset(encode_out, 'A', sizeof(encode_out));
    encoded = cobs_encode(encode_out, sizeof(encode_out),
                          cobs_tests[i].data_ptr, cobs_tests[i].data_len);
  }
  printf("encoded length: %d\n", encoded.out_len);
  // fixme : program crash
  //printf("encoded data: %s\n", encode_out);

  // -- id : convert uint8 to byte
  // fixme : id maximum too low
  McuUtility_constrain(temperature_measurement->id, 0, 255);

  // -- temperature : convert float to byte
  // - 1% resolution for tmp117 with +/- 0.1°C accuracy
  // fixme : data loss conversion (eg. 26.03 -> 2600)
  uint8_t data_16LE_byte[2] = {0};
  McuUtility_constrain((int32_t)temperature_measurement->temperature, -20, 150);
  uint16_t temperature = (uint16_t)(temperature_measurement->temperature * 100);
  // printf("temperature as uint16: %u\n", temperature);
  McuUtility_SetValue16LE(temperature, data_16LE_byte);
  print_bits_of_byte(data_16LE_byte[1], false);
  print_bits_of_byte(data_16LE_byte[0], false);
  printf("\n");
  printf("send temperature as bytes\n");

  rc232_tx_packet_bytes(data_16LE_byte[0], dryrun);
  rc232_tx_packet_bytes(data_16LE_byte[1], dryrun);
}

/**
 * @brief Send test message.
 */
void radio_send_test(void) { rc232_tx_packet_string("hello world", false); }

static void print_bits_of_byte(uint8_t byte, bool print) {
  for (int i = 7; i >= 0; i--) {
    if (print) {
      printf("%c", (byte & (1 << i)) ? '1' : '0');
    }
    McuLog_trace("%c", (byte & (1 << i)) ? '1' : '0');
  }
}