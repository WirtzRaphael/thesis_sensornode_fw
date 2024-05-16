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
 * todo : function headers
 */
#include "radio.h"
#include "McuLib.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "McuUtility.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/unique_id.h"
#include "semphr.h"
#include "yahdlc.h"
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

#define RADIO_LOG_OUTPUT printf
// #define RADIO_LOG_OUTPUT McuLog_trace // fixme : some values missing

#ifndef dimof
  #define dimof(X) (sizeof(X) / sizeof((X)[0]))
#endif

// todo refactor (get set ?, no define) / Init ?
static rf_settings_t rf_settings = {
    .destination_address = RF_DESTINATION_ADDR_DEFAULT,
    .source_address = 99,
    .channel_id = RF_CHANNEL_DEFAULT,
    .channel_start = RF_CHANNEL_MIN,
    .channel_end = RF_CHANNEL_MAX,
};
char rf_destination_address = RF_DESTINATION_ADDR_DEFAULT;

// check : init temperature values etc.?
static radio_data_temperature_t data_temperature = {
    .index = 0,
    .temperature_index = 1,
};

static uint16_t radio_time_intervals_ms = 500;

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
    // todo : readout buffer and decode, decompose
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
      // radio_send_temperature_as_string(&temperature_measurement_sensor1,
      // true);
      radio_send_temperature_as_bytes(xQueue_temperature_sensor_1, false);
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
                  2500 / sizeof(StackType_t), /* task stack size */
                  (void *)NULL,         /* optional task startup argument */
                  tskIDLE_PRIORITY + 2, /* initial priority */
                  (TaskHandle_t *)NULL  /* optional task handle to create */
                  ) != pdPASS) {
    for (;;) {
    } /* error! probably out of memory */
  }
}

/**
 * @brief Get radio destination address.
 *
 * @return char
 */
char radio_get_rf_destination_address(void) { return rf_settings.destination_address; }

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
  rf_settings.channel_start = RC1701_RF_CHANNEL_MIN;
  rf_settings.channel_end = RC1701_RF_CHANNEL_MAX;
#else // only default channel
  rf_settings.channel_start = RF_CHANNEL_DEFAULT;
  rf_settings.channel_end = RF_CHANNEL_DEFAULT;
#endif

  // check channel range
  if (!(rf_settings.channel_start >= 1 && rf_settings.channel_end <= 10)) {
    McuLog_error("radio : invalid channel range\n");
    return;
  }

  // send request to broadcast address
  rc232_config_destination_address(RC232_BROADCAST_ADDRESS);
  for (int i = rf_settings.channel_start; i <= rf_settings.channel_end; i++) {
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
  rc232_config_rf_channel_number(rf_settings.channel_id);
}

/**
 * @brief Send authentication request.
 */
static void radio_send_authentication_request(void) {
  // todo : HDLC-Lite (FRAMING PROTOCOL, high-level data link
  // control) todo : protocol authentication request todo : send -> tx (rc232)
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
 * NOTE: DO NOT USE
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
  RADIO_LOG_OUTPUT("send temperature: %f\n",
                   (temperature_measurement->temperature));
  RADIO_LOG_OUTPUT("send temperature (buffer): %s\n", buffer);

  // Send
  rc232_tx_packet_string(buffer, dryrun);
}

/**
 * @brief Send temperature values as bytes.
 *
 * - use HDLC-Lite for encoding
 * -
 * todo : change, compress format / algorithm to reduce number of temperatures
 * todo : typedef / enum
 * todo : refactor
 * reference :
 * https://github.com/bang-olufsen/yahdlc/blob/master/C/test/yahdlc_test.cpp
 */
error_t radio_send_temperature_as_bytes(QueueHandle_t xQueue_temperature,
                                        bool dryrun) {
  // -- temperature values
  int hdlc_ret;
  yahdlc_control_t control;
  control.frame = YAHDLC_FRAME_DATA;
  // control.seq_no = 0;
  char send_data[16];
  char frame_data[24];
  unsigned int frame_length = 0;

  // -- data to send
  // Up to 0x70 to keep below the values to be escaped
  radio_data_temperature_t data_temperature;
  //  data_temperature.info_field = SENSORS_TEMPERATURE_1;
  error_t error;

  // -- fill data to send
  // Content info field
  // note : direct usage?
  send_data[0] = SENSORS_TEMPERATURE_1;
  data_temperature.index += 1;

  // Measurement values
  // reads multiple values from queue until buffer full or queue empty
  for (data_temperature.index;
       data_temperature.index < 2 * RADIO_TEMPERATURE_VALUES;
       data_temperature.index += 2) {
    McuLog_trace("index : %d\n", index);
    if (data_temperature.index > sizeof(send_data)) {
      McuLog_error("[radio] buffer overflow for data to send\n");
      return ERR_OVERFLOW;
    }
    error = sensors_temperature_xQueue_receive(xQueue_temperature,
                                               &data_temperature.measurement);
    if (!(error == ERR_OK)) {
      printf("[radio] No new temperature value received\n");
      continue;
    }
    convert_temperature_to_byte(data_temperature.measurement_byte,
                                &data_temperature.measurement);
    printf("temperature: %f\n", data_temperature.measurement.temperature);
    send_data[data_temperature.index] = data_temperature.measurement_byte[0];
    send_data[data_temperature.index + 1] =
        data_temperature.measurement_byte[1];
  }
  log_hdlc_data(send_data, sizeof(send_data));

  // -- encoding
  // Initialize control field structure and create frame
  control.frame = YAHDLC_FRAME_DATA;
  hdlc_ret = yahdlc_frame_data(&control, send_data, sizeof(send_data),
                               frame_data, &frame_length);
  if (hdlc_ret != 0) {
    McuLog_error("hdlc encode frame data error\n");
    return ERR_FRAMING;
  }
  log_hdlc_encoded(frame_data, frame_length);

#if RADIO_DEBUG_DECODE
  // -- decode
  char recv_data[24];
  unsigned int recv_length = 0; // todo : data type
  // Decode the data up to end flag sequence byte which should return no valid
  hdlc_ret = yahdlc_get_data(&control, frame_data, frame_length - 1, recv_data,
                             &recv_length);
  // Decode the end flag sequence byte which should result in a decoded frame
  hdlc_ret = yahdlc_get_data(&control, &frame_data[frame_length - 1], 1,
                             recv_data, &recv_length);
  if (hdlc_ret != 0) {
    McuLog_error("hdlc decode frame data error\n");
    return ERR_FRAMING;
  }
  log_hdlc_decoded(recv_data, recv_length);
#endif

  rc232_tx_packet_bytes(send_data, sizeof(send_data), true);
  return ERR_OK;
}

void radio_encoding_hdlc_example(void) {
  // -- temperature values
  int ret;
  yahdlc_control_t control;
  control.frame = YAHDLC_FRAME_DATA;
  char send_data[16];
  char frame_data[24];
  unsigned int i, frame_length = 0;

  // Initialize data to be send with random values (up to 0x70 to keep below
  // the values to be escaped)
  printf("Data to be sent: ");
  for (i = 0; i < sizeof(send_data); i++) {
    // send_data[i] = (char)(rand() % 0x70);
    printf("%c", send_data[i]);
    printf("%d", send_data[i]);
  }
  printf("\n");

  // Initialize control field structure and create frame
  control.frame = YAHDLC_FRAME_DATA;
  ret = yahdlc_frame_data(&control, send_data, sizeof(send_data), frame_data,
                          &frame_length);

  printf("Data frame: ");
  for (i = 0; i < frame_length; i++) {
    printf("%c", frame_data[i]);
    printf("%d", frame_data[i]);
  }
  printf("\n");

  char recv_data[24];
  unsigned int recv_length = 0; // todo : data type
  // Decode the data up to end flag sequence byte which should return no valid
  // messages error
  ret = yahdlc_get_data(&control, frame_data, frame_length - 1, recv_data,
                        &recv_length);
  // Now decode the end flag sequence byte
  // which should result in a decoded frame
  ret = yahdlc_get_data(&control, &frame_data[frame_length - 1], 1, recv_data,
                        &recv_length);
  printf("Data received: ");
  for (i = 0; i < recv_length; i++) {
    printf("%c", recv_data[i]);
    printf("%d", recv_data[i]);
  }
  printf("\n");
}

/**
 * @brief Send test message.
 */
void radio_send_test_string(void) {
  rc232_tx_packet_string("hello world", false);
}

static void print_bits_of_byte(uint8_t byte, bool print) {
  for (int i = 7; i >= 0; i--) {
    if (print) {
      printf("%c", (byte & (1 << i)) ? '1' : '0');
    }
    printf("%c", (byte & (1 << i)) ? '1' : '0');
  }
  if (print) {
    printf("\n");
  }
}

/**
 * @brief Convert temperature to byte.
 *
 * @param data_16LE_byte
 * @param temperature_measurement
 * @note 16 bit little endian
 * @note 1% resolution for tmp117 with +/- 0.1°C accuracy
 * fixme : data loss conversion (eg. 26.03 -> 2600) / overflow multiplication?
 * todo : check accuracy 16/32 bit
 * todo : change range, start values from 0 and in range of sensor.
 */
static void convert_temperature_to_byte(
    uint8_t *data_16LE_byte,
    temperature_measurement_t *temperature_measurement) {
  // todo : constrains int32 correct? (min -20°C, max 150°C)
  McuUtility_constrain((int32_t)temperature_measurement->temperature, -20, 150);
  uint16_t temperature = (uint16_t)(temperature_measurement->temperature * 100);
  McuUtility_SetValue16LE(temperature, data_16LE_byte);
  RADIO_LOG_OUTPUT("[send] ==> sensor: Temperature \n");
  RADIO_LOG_OUTPUT("[send]  -> converted to byte:\n");
  print_bits_of_byte(data_16LE_byte[1], true);
  print_bits_of_byte(data_16LE_byte[0], true);
}

/**
 * @brief Log hdlc encoding data to send
 *
 * @param data_ptr
 * @param data_len
 */
static void log_hdlc_data(char *data_ptr, size_t data_len) {
  RADIO_LOG_OUTPUT("[hdlc] ==> Data to send \n");
  RADIO_LOG_OUTPUT("[hdlc]  -> char : ");
  log_buffer_as_char(data_ptr, data_len);
  RADIO_LOG_OUTPUT("[hdlc]  -> int : ");
  log_buffer_as_int(data_ptr, data_len);
  RADIO_LOG_OUTPUT("[hdlc]  -> bin : ");
  print_bits_of_byte(*(data_ptr), true);
  RADIO_LOG_OUTPUT("[hdlc]  -> length: %d\n", data_len);
}

/**
 * @brief Log hdlc encoded data
 *
 * @param encoded_ptr
 * @param encoded_len
 */
static void log_hdlc_encoded(char *encoded_ptr, size_t encoded_len) {
  RADIO_LOG_OUTPUT("[hdlc] ==> Data encoded \n");
  RADIO_LOG_OUTPUT("[hdlc]  -> char : ");
  log_buffer_as_char(encoded_ptr, encoded_len);
  RADIO_LOG_OUTPUT("[hdlc]  -> int : ");
  log_buffer_as_int(encoded_ptr, encoded_len);
  RADIO_LOG_OUTPUT("[hdlc]  -> bin : ");
  print_bits_of_byte(*(encoded_ptr), true);
  RADIO_LOG_OUTPUT("[hdlc]  -> length: %d\n", encoded_len);
}

/**
 * @brief Log hdlc decoded data
 *
 * @param decoded_ptr
 * @param decoded_len
 */
static void log_hdlc_decoded(char *decoded_ptr, size_t decoded_len) {
  RADIO_LOG_OUTPUT("[hdlc] ==> Data decoded \n");
  RADIO_LOG_OUTPUT("[hdlc]  -> char : ");
  log_buffer_as_char(decoded_ptr, decoded_len);
  RADIO_LOG_OUTPUT("[hdlc]  -> int : ");
  log_buffer_as_int(decoded_ptr, decoded_len);
  RADIO_LOG_OUTPUT("[hdlc]  -> bin : ");
  print_bits_of_byte(*(decoded_ptr), true);
  RADIO_LOG_OUTPUT("[hdlc]  -> length: %d\n", decoded_len);
}

/**
 * @brief Log buffer as char
 *
 * @param buffer
 * @param length
 */
static void log_buffer_as_char(char *buffer, size_t length) {
  for (size_t i = 0; i < length; i++) {
    printf("|%c", (char)buffer[i]);
  }
  printf("\n");
}

/**
 * @brief Log buffer as int
 *
 * @param buffer
 * @param length
 */
static void log_buffer_as_int(char *buffer, size_t length) {
  for (size_t i = 0; i < length; i++) {
    printf("|%d", (char)buffer[i]);
  }
  printf("\n");
}
