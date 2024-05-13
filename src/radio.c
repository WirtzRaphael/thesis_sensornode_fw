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
 * todo : temperature sensor values
 */
#include "radio.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "McuUtility.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/unique_id.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_types.h>
//
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

int rf_channel_start = 0;
int rf_channel_end = 0;
char rf_channel = RF_CHANNEL_DEFAULT;
char rf_destination_address = RF_DESTINATION_ADDR_DEFAULT;

static uint16_t radio_time_intervals_ms = 5000;

pico_unique_board_id_t pico_uid = {0};
char pico_uid_string[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 +
                     1]; // Each byte to two hex chars + null terminator

static void vRadioTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // periodic task
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(radio_time_intervals_ms));

    // todo : different operations (send, buffer management, authentication, fw
    // download/update)
    // printf("===== radio task\n");
    // sensors_print_temperature_xQueue_latest(xQueue_temperature_sensor_1);
    // sensors_print_temperature_xQueue_latest(xQueue_temperature_sensor_2);
    temperature_measurement_t temperature_measurement_sensor1 = {0, 0, 0};
    sensors_temperature_xQueue_receive(xQueue_temperature_sensor_1,
                                       &temperature_measurement_sensor1);
    radio_send_temperature_as_string(&temperature_measurement_sensor1, true);
    if (xSemaphoreTake(xButtonASemaphore, portMAX_DELAY) == pdTRUE) {
      // Button was pressed, perform action
      printf("[radio] Semaphore take A\n");
    }
    // todo : sensor 2

    // printf("radio task end =====\n");
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
     * todo : hscl lite protocol
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
  rc232_tx_string(frame_flag, ACTIVATE_RF);
  // message
  rc232_tx_string(payload, ACTIVATE_RF);
  // frame delimiter flag
  rc232_tx_string(frame_flag, ACTIVATE_RF);
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
    temperature_measurement_t *temperature_measurement) {
  // todo : send temperature values
  uint8_t buffer[32];
  McuUtility_NumFloatToStr(buffer, sizeof(buffer),
                           temperature_measurement->temperature, 2);

#if PRINTF_RF
  printf("send temperature: %f\n", (temperature_measurement->temperature));
  printf("send temperature (buffer): %s\n", buffer);
#endif
  rc232_tx_string(buffer, true);
  // rc232_tx_string("temperature values");
}

/**
 * @brief Send test message.
 */
void radio_send_test(void) { rc232_tx_string("hello world", false); }
