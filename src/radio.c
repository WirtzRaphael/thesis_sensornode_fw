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
 * todo : crc (-> fcs ?)
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
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types.h>

//
#include "application.h"
#include "rc232.h"
#include "sensors.h"

#define PROTOCOL_VERSION         (1)
#define PROTOCOL_AUTH_SIZE_BYTES (10)
#define RF_CHANNEL_DEFAULT       (1)

// Use fix channel or scan channels
#define SCAN_CHANNELS_FOR_CONNECTION (0)
#define TRANSMISSION_IN_BYTES        (0)
#define DEACTIVATE_RF                (1)

#define RADIO_LOG_OUTPUT printf
// #define RADIO_LOG_OUTPUT McuLog_trace // fixme : some values missing

#ifndef dimof
  #define dimof(X) (sizeof(X) / sizeof((X)[0]))
#endif

// todo refactor (get set ?, no define) / Init ?
static rf_settings_t rf_settings = {
    .destination_address = 20,
    .source_address = 99,
    .channel_id = 1,
    .channel_default = 1,
    .channel_start = 1,
    .channel_end = 10,
};

// check : init temperature values etc.?
static radio_data_temperature_t data_temperature = {
    .index = 0,
    .temperature_index = 1,
};

static uint16_t radio_time_intervals_ms = 500;

pico_unique_board_id_t pico_uid = {0};

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
      radio_authentication();
    }
    if (xSemaphoreTake(xButtonBSemaphore, xButtonSemaphore) == pdTRUE) {
      printf("[radio] Semaphore take Button B\n");
      printf("[radio] Send Temperature 1 \n");
      // fixme : send all values and empty values otherwise
      data_info_field_t data_info_temperature_1 = {PROTOCOL_VERSION,
                                                   DATA_SENSORS_TEMPERATURE_1};
      // todo : refactor : readout temperatures into array and pass to radio
      // send fixme : interface readout queue and send values here !
      radio_send_temperature_as_bytes(xQueue_temperature_sensor_1,
                                      data_info_temperature_1, false);

      printf("[radio] Send Temperature 2 \n");
      data_info_field_t data_info_temperature_2 = {PROTOCOL_VERSION,
                                                   DATA_SENSORS_TEMPERATURE_2};
      radio_send_temperature_as_bytes(xQueue_temperature_sensor_2,
                                      data_info_temperature_2, false);
    }

    //  printf("radio killed the video star.");
  }
}

/**
 * @brief Initialize radio.
 */
void radio_init(void) {
  pico_get_unique_board_id(&pico_uid);
  McuLog_trace("pico unique id: %d \n", pico_uid.id);

  // todo : stack depth (2500 increased)
  // BaseType_t xReturned;
  // TaskHandle_t xHandle = NULL;
  if (xTaskCreate(vRadioTask, /* pointer to the task */
                  "radio",    /* task name for kernel awareness debugging */
                  3500 / sizeof(StackType_t), /* task stack size */
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
char radio_get_rf_destination_address(void) {
  return rf_settings.destination_address;
}

/**
 * @brief Scan radio network and authenticate.
 *
 * - Scans channels (optional)
 * - Send authentication request
 * - Waiting for acknowledge
 * -
 * todo : settings from response
 * - Free UID network (optional)
 * - UID gateway -> DID
 * - time sync (optional)
 * todo : refactor when not direct radio buffer used i.e. radio task
 * fixme : buffer read infinit wait -> reset (problematic ?)
 */
void radio_authentication(void) {
  rc232_rx_read_buffer_full(); // empty buffer

  RADIO_LOG_OUTPUT("[radio] ==> Scan authentication\n");
#if SCAN_CHANNELS_FOR_CONNECTION
  rf_settings.channel_start = RC1701_RF_CHANNEL_MIN;
  rf_settings.channel_end = RC1701_RF_CHANNEL_MAX;
#else // only default channel
  rf_settings.channel_start = rf_settings.channel_default;
  rf_settings.channel_end = rf_settings.channel_default;
#endif
  // check channel range
  if (!(rf_settings.channel_start >= 1 && rf_settings.channel_end <= 10)) {
    McuLog_error("radio : invalid channel range\n");
    return;
  }
  // send request to broadcast address
  rc232_config_destination_address(RC232_BROADCAST_ADDRESS);
  uint8_t i = rf_settings.channel_start;
  for (i; i <= rf_settings.channel_end; i++) {
    RADIO_LOG_OUTPUT("[radio]  -> channel %d\n", i);
    rc232_config_rf_channel_number(i);
    radio_authentication_request();
    RADIO_LOG_OUTPUT("[radio]  -> wait for response %d\n", i);
    radio_authentication_wait_for_response(2000);
  }

  // todo : finish handshake (more steps)
  // todo : settings from response / time sync?

  // reset to default address
  rc232_config_destination_address(rf_settings.destination_address);
  rc232_config_rf_channel_number(rf_settings.channel_id);
}

/**
 * @brief Send authentication request.
 * todo : refactor : sub functions / avoid duplicate code
 */
static error_t radio_authentication_request(void) {
  int hdlc_ret;
  yahdlc_control_t control;
  control.frame = YAHDLC_FRAME_DATA;
  // control.seq_no = 0;
  char send_data[16];
  char frame_data[24];
  unsigned int frame_length = 0;
  uint8_t index = 0;

  // Payload content info
  data_info_field_t data_info_field = {PROTOCOL_VERSION,
                                       DATA_AUTHENTICATION_REQUEST};
  // todo : index refactor
  pack_data_info_field(&data_info_field, send_data[0]);
  index++;
  send_data[1] = 255; // todo : receiver address
  index++;

  // Payload content
  for (index; index < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; index++) {
    send_data[index] = pico_uid.id[index];
  }
  log_hdlc_data(send_data, sizeof(send_data));
  control.frame = YAHDLC_FRAME_DATA;
  hdlc_ret = yahdlc_frame_data(&control, send_data, sizeof(send_data),
                               frame_data, &frame_length);
  if (hdlc_ret != 0) {
    McuLog_error("[auth] hdlc encode frame data error\n");
    return ERR_FRAMING;
  }
  log_hdlc_encoded(frame_data, frame_length);

#if RADIO_DEBUG_DECODE
  RADIO_LOG_OUTPUT("[auth] hdlc decode frame data\n");
  // todo : sub function decode ?
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
    McuLog_error("[decode] hdlc decode frame data error\n");
    return ERR_FRAMING;
  }
  log_hdlc_decoded(recv_data, recv_length);
#endif

  rc232_tx_packet_bytes(send_data, sizeof(send_data), DEACTIVATE_RF);
  return ERR_OK;
}

/**
 * @brief Wait for authentication response.
 *
 * @return error_t
 * todo : acknowledge hdlc frame ?
 * todo : error arbitration ?
 * todo : frame length (max) value
 */
static error_t radio_authentication_wait_for_response(uint32_t timeout_ms) {
  // uint8_t buffer[PROTOCOL_AUTH_SIZE_BYTES] = {0};
  error_t err;
  bool response = false;
  RADIO_LOG_OUTPUT("[auth] ==> Wait for response\n");
  // wait until response or timeout
  uint8_t t_poll_ms = 10;
  for (int t = 0; t <= timeout_ms || response == true; t = t + t_poll_ms) {
    int hdlc_ret;
    yahdlc_control_t control;
    control.frame = YAHDLC_FRAME_ACK;
    // control.frame = YAHDLC_FRAME_DATA;
    unsigned int frame_length = 0;
    char frame_data[RADIO_BYTES_TO_BITS(4)];
    vTaskDelay(pdMS_TO_TICKS(t_poll_ms));

    // todo : readout buffer / no test data
    test_data_encoded(frame_data, 24);
    /*
    err = rc232_rx_read_bytes(frame_data, 4);
    if (err == !ERR_OK) {
      continue;
    }
    */
    log_hdlc_encoded(frame_data, sizeof(frame_data));

    // -- decode
    char recv_data[RADIO_BYTES_TO_BITS(4)];
    unsigned int recv_length = 0;
    // Decode the data up to end flag sequence byte which should return no valid
    hdlc_ret = yahdlc_get_data(&control, frame_data, frame_length - 1,
                               recv_data, &recv_length);
    // Decode the end flag sequence byte which should result in a decoded frame
    hdlc_ret = yahdlc_get_data(&control, &frame_data[frame_length - 1], 1,
                               recv_data, &recv_length);
    if (hdlc_ret != 0) {
      RADIO_LOG_OUTPUT("hdlc decode frame data error\n");
      continue;
    }
    log_hdlc_decoded(recv_data, sizeof(recv_data));
    yahdlc_get_data_reset();

    // -- decompose payload
    data_info_field_t data_info_field;
    unpack_data_info_field(&data_info_field, recv_data[0]);

    // todo : - get receiver address
    // todo : - board id
    if (data_info_field.data_content == DATA_AUTHENTICATION_ACK) {
      RADIO_LOG_OUTPUT("radio : acknowledge received\n");
      // todo : action after acknowledge (payload values, set uid, etc.)
      // channel) receive response
      // - Free UID network (optional)
      // - UID gateway -> DID
      // - time sync (optional)
      response = true;
      return ERR_OK;
    }
  }
  return ERR_FAILED;
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
 * todo : fill all buffer values / overwrite existing buffer values
 * reference :
 * https://github.com/bang-olufsen/yahdlc/blob/master/C/test/yahdlc_test.cpp
 */
error_t radio_send_temperature_as_bytes(QueueHandle_t xQueue_temperature,
                                        data_info_field_t data_info_field,
                                        bool dryrun) {
  if (!data_info_field.protocol_version || !data_info_field.data_content) {
    McuLog_error("[radio] invalid data info field parameters\n");
    return ERR_PARAM_DATA;
  }
  int hdlc_ret;
  yahdlc_control_t control;
  control.frame = YAHDLC_FRAME_DATA;
  // control.seq_no = 0;
  char send_data[16];
  char frame_data[24];
  unsigned int frame_length = 0;
  data_temperature.index = 0; // reset index
  error_t error_resp;

  pack_data_info_field(&data_info_field, send_data[0]);
  data_temperature.index++;
  send_data[1] = 255; // todo : receiver address
  data_temperature.index++;

  // Measurement values
  // reads multiple values from queue until buffer full or queue empty
  // note : number of values multiplied by the size
  for (data_temperature.index;
       data_temperature.index <= 2 * RADIO_TEMPERATURE_VALUES;
       data_temperature.index += 2) {
    McuLog_trace("index : %d\n", index);
    if (data_temperature.index > sizeof(send_data)) {
      McuLog_error("[radio] buffer overflow for data to send\n");
      return ERR_OVERFLOW;
    }
    error_resp = sensors_temperature_xQueue_receive(
        xQueue_temperature, &data_temperature.measurement);
    if (!(error_resp == ERR_OK)) {
      printf("[radio] No new temperature value received\n");
      // fill values
      send_data[data_temperature.index] = 0;
      send_data[data_temperature.index + 1] = 0;
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

  if (data_temperature.index <= data_temperature.temperature_index) {
    McuLog_error("[radio] No new temperature value received\n");
    return ERR_NOTAVAIL;
  }

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
  unsigned int recv_length = 0;
  decode_hdlc_frame(&control, frame_data, frame_length, recv_data,
                    &recv_length);
#endif

  rc232_tx_packet_bytes(send_data, sizeof(send_data), DEACTIVATE_RF);
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
 * @brief Pack data info field for payload.
 *
 * @param field_src
 * @param data_info_field
 */
static void pack_data_info_field(data_info_field_t *field_src,
                                 uint8_t data_info_field) {
  data_info_field =
      (field_src->protocol_version | field_src->data_content << 3);
  RADIO_LOG_OUTPUT("[hdlc] ==> Pack data info field \n");
  RADIO_LOG_OUTPUT("[hdlc]  -> protocol version: %d\n",
                   field_src->protocol_version);
  RADIO_LOG_OUTPUT("[hdlc]  -> data content: %d\n", field_src->data_content);
  RADIO_LOG_OUTPUT("[hdlc]  -> packed: %d\n", data_info_field);
}

/**
 * @brief Unpack data info field from payload.
 *
 * @param field_dest
 * @param data_info_field
 */
static void unpack_data_info_field(data_info_field_t *field_dest,
                                   uint8_t data_info_field) {
  field_dest->data_content = data_info_field & 0x07;
  field_dest->protocol_version = data_info_field >> 3;
  RADIO_LOG_OUTPUT("[hdlc] ==> Unpack data info field \n");
  RADIO_LOG_OUTPUT("[hdlc]  -> packed: %d\n", data_info_field);
  RADIO_LOG_OUTPUT("[hdlc]  -> protocol version: %d\n",
                   field_dest->protocol_version);
  RADIO_LOG_OUTPUT("[hdlc]  -> data content: %d\n", field_dest->data_content);
}

/**
 * @brief Decode hdlc frame data wrapper function.
 *
 * @param control
 * @param frame_data
 * @param frame_length
 * @param recv_data
 * @param recv_length
 * @return int : error code hdlc
 */
error_t decode_hdlc_frame(yahdlc_control_t *control, char *frame_data,
                          size_t frame_length, char *recv_data,
                          size_t *recv_length) {
  uint8_t hdlc_ret;
  // Decode the data up to end flag sequence byte which should return no valid
  hdlc_ret = yahdlc_get_data(control, frame_data, frame_length - 1, recv_data,
                             recv_length);
  // Decode the end flag sequence byte which should result in a decoded frame
  hdlc_ret = yahdlc_get_data(control, &frame_data[frame_length - 1], 1,
                             recv_data, recv_length);
  if (hdlc_ret != 0) {
    McuLog_error("hdlc decode frame data error\n");
    return ERR_FRAMING;
  }
  log_hdlc_decoded(recv_data, *recv_length);
  return ERR_OK;
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
  // RADIO_LOG_OUTPUT("[hdlc]  -> char : ");
  // log_buffer_as_char(encoded_ptr, encoded_len);
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
  // RADIO_LOG_OUTPUT("[hdlc]  -> char : ");
  // log_buffer_as_char(decoded_ptr, decoded_len);
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

/**
 * @brief Encoded hdlc frame data for testing
 *
 * @param frame_data
 * @param option_length : select test data
 */
static void test_data_encoded(char *frame_data, uint8_t option_length) {
  // - encoded values
  if (option_length == 30) {
    // one frame exactly
    char frame_data[30] = {23,  30, 126, 255, 16,  25,  255, 0, 0,
                           233, 9,  234, 9,   233, 9,   233, 9, 170,
                           68,  0,  16,  213, 15,  126, 23,  3};
    strncpy(frame_data, frame_data, 30);
  } else if (option_length == 24) {
    // one frame with additional data
    char frame_data[24] = {126, 255, 16,  25, 255, 0,  0, 233, 9,   234, 9,
                           233, 9,   233, 9,  170, 68, 0, 16,  213, 15,  126};
    strncpy(frame_data, frame_data, 24);
  } else {
    strncpy(frame_data, "hello world", 11);
  }
  /* - decoded
  {1,  255, 76, 49, 27,  130, 71,  50,
                         25, 134, 0,  16, 165, 165, 165, 165};
                         */
}