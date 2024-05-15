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

#define RADIO_LOG_OUTPUT printf
// #define RADIO_LOG_OUTPUT McuLog_trace

#ifndef dimof
  #define dimof(X) (sizeof(X) / sizeof((X)[0]))
#endif

int rf_channel_start = 0;
int rf_channel_end = 0;
char rf_channel = RF_CHANNEL_DEFAULT;
char rf_destination_address = RF_DESTINATION_ADDR_DEFAULT;

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
                  2500 / sizeof(StackType_t), /* task stack size */
                  (void *)NULL,         /* optional task startup argument */
                  tskIDLE_PRIORITY + 2, /* initial priority */
                  (TaskHandle_t *)NULL  /* optional task handle to create */
                  ) != pdPASS) {
    for (;;) {
    } /* error! probably out of memory */
  }
}

char radio_get_rf_destination_address(void) { return rf_destination_address; }

// todo : error log encoding result
/**
 * @brief Encode data as cobs protocol.
 *
 * @param encoded_payload_ptr
 * @param encoded_payload_len
 * @param payload_bytes
 * @param payload_bytes_len
 * @param encoded_result_payload
 */
void radio_encode(void *encoded_payload_ptr, size_t encoded_payload_len,
                  cobs_data *payload_bytes, size_t payload_bytes_len,
                  cobs_encode_result *encoded_result_payload) {
  //                                const void *payload_bytes_ptr,
  //                                size_t payload_bytes_len) {
  // cobs_encode_result encoded_result_payload;
  size_t i;
  for (i = 0; i < payload_bytes_len; i++) {
    // cobs_encode_result encoded_result_payload;

    uint8_t *encoded_payload_byte_ptr = (uint8_t *)encoded_payload_ptr;
    encoded_result_payload[i] =
        cobs_encode(encoded_payload_ptr, encoded_payload_len,
                    payload_bytes[i].data_ptr, payload_bytes[i].data_len);

    // log_payload(payload_bytes[i].data_ptr, payload_bytes[i].data_len );
    // log_encoded(encoded_payload_byte_ptr, encoded_result_payload[i].out_len);
  }
}

// todo : add error handling ? logs.
// todo : refactor parameters
// fixme : only decoding one value
void radio_decode(
    // cobs_data *decoded_payload_ptr,
    void *decoded_payload_ptr, size_t decoded_payload_len,
    void *encoded_payload_ptr, size_t encoded_payload_len,
    cobs_encode_result *encoded_result_payload,
    cobs_decode_result *decoded_result_payload,
    // const void *encoded_payload,
    size_t payload_bytes_len) {
  // cobs_decode_result decoded_result_payload;
  uint8_t *decoded_payload_byte_ptr = (uint8_t *)decoded_payload_ptr;
  uint8_t *encoded_payload_byte_ptr = (uint8_t *)encoded_payload_ptr;

  size_t i;
  for (i = 0; i < payload_bytes_len; i++) {
    decoded_result_payload[i] =
        cobs_decode(decoded_payload_ptr, decoded_payload_len,
                    encoded_payload_byte_ptr, encoded_payload_len);
  }

  // input data
  for (i = 0; i < payload_bytes_len; i++) {
    printf("[decode] encoded : %d \n", encoded_payload_ptr);
  }
  printf("[decode] encoded length: %d\n", encoded_payload_len);

  printf("[decode] ==> decoding \n");
  // decoded data
  for (i = 0; i < decoded_result_payload->out_len; i++) {
    printf("[decode]  -> decoded [%d] : %d | ", i, decoded_payload_byte_ptr[i]);
    print_bits_of_byte(decoded_payload_byte_ptr[i], true);
  }
  printf("[decode]  -> decoded length: %d\n", decoded_result_payload->out_len);
  printf("[decode]\n");
}

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
  // todo : cobs instead of HDLC-Lite (FRAMING PROTOCOL, high-level data link
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
 * todo : cobs protocol
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
  RADIO_LOG_OUTPUT("send temperature: %f\n",
                   (temperature_measurement->temperature));
  RADIO_LOG_OUTPUT("send temperature (buffer): %s\n", buffer);

  // Send
  rc232_tx_packet_string(buffer, dryrun);
}

/**
 * @brief Send temperature values as bytes.
 *
 * - use cobs for encoding
 * todo : refactor
 * todo : time information
 * todo : change, compress format / algorithm to reduce number of temperatures
 * (eg only diffs) diffs) todo : error code return
 */
void radio_send_temperature_as_bytes(
    temperature_measurement_t *temperature_measurement, bool dryrun) {
  // -- id : convert uint8 to byte
  // fixme : id maximum too low
  McuUtility_constrain(temperature_measurement->id, 0, 255);

  // -- temperature : convert float to byte
  // - 1% resolution for tmp117 with +/- 0.1°C accuracy
  // fixme : data loss conversion (eg. 26.03 -> 2600)
  uint8_t data_16LE_byte[2] = {0};
  McuUtility_constrain((int32_t)temperature_measurement->temperature, -20, 150);
  uint16_t temperature = (uint16_t)(temperature_measurement->temperature * 100);
  McuUtility_SetValue16LE(temperature, data_16LE_byte);
  RADIO_LOG_OUTPUT(" ==> Sensor - Temperature \n");
  RADIO_LOG_OUTPUT("  -> converted to byte:\n");
  print_bits_of_byte(data_16LE_byte[1], true);
  print_bits_of_byte(data_16LE_byte[0], true);

  // -- payload
  // todo : add more
  // cobs_data cobs_payload[] = {data_16LE_byte, sizeof(data_16LE_byte)};
  cobs_data payload_bytes[] = {{&data_16LE_byte[0], 1},
                               {&data_16LE_byte[1], 1}};

  // -- encode
  // fixme : stackoverflow when (multiple) executions
  uint8_t encoded_payload[COBS_ENCODE_DST_BUF_LEN_MAX(50)];
  cobs_encode_result encoded_result_payload;
  uint8_t decoded_payload[50];
  cobs_decode_result decode_result_payload;
  size_t i;

  // each byte of the payload
  for (i = 0; i < dimof(payload_bytes); i++) {
    // encoded_result_payload[i] =
    RADIO_LOG_OUTPUT("[send] ==> encoding payload \n");
    encoded_result_payload =
        cobs_encode(encoded_payload, sizeof(encoded_payload),
                    payload_bytes[i].data_ptr, payload_bytes[i].data_len);

    RADIO_LOG_OUTPUT("[send] ==> decoding payload\n");
    decode_result_payload =
        cobs_decode(decoded_payload, sizeof(decoded_payload), encoded_payload,
                    encoded_result_payload.out_len);

    RADIO_LOG_OUTPUT("[send] ==> encoding summary\n");
    log_cobs_payload(payload_bytes, i);
    log_cobs_encoded(encoded_payload, encoded_result_payload);
    log_cobs_decoded(decoded_payload, decode_result_payload);
  }

  // fixme : access results via index ?
  RADIO_LOG_OUTPUT("[decoded]  -> data per index : %u \n", decoded_payload[0]);
  RADIO_LOG_OUTPUT("[decoded]  -> data per index : %u \n", decoded_payload[1]);

  // -- send
  // todo : send whole encoded package (use length info)
  // todo : send mulitple bytes in one function / transmission
  rc232_tx_packet_bytes(encoded_payload[0], dryrun);
  rc232_tx_packet_bytes(encoded_payload[1], dryrun);
  rc232_tx_packet_bytes(encoded_payload[2], dryrun);
  rc232_tx_packet_bytes(encoded_payload[3], dryrun);
}

/**
 * @brief Send test message.
 */
void radio_send_test(void) { rc232_tx_packet_string("hello world", false); }

static void print_bits_of_byte(uint8_t byte, bool print) {
  for (int i = 7; i >= 0; i--) {
    if (print) {
      RADIO_LOG_OUTPUT("%c", (byte & (1 << i)) ? '1' : '0');
    }
    RADIO_LOG_OUTPUT("%c", (byte & (1 << i)) ? '1' : '0');
  }
  if (print) {
    RADIO_LOG_OUTPUT("\n");
  }
}

// todo : change to mcu log
static void log_cobs_payload(cobs_data *payload_cobs, size_t index) {
  RADIO_LOG_OUTPUT("[payload]  -> data : %d \n",
                   payload_cobs[index].data_ptr[0]);
  RADIO_LOG_OUTPUT("[payload]  -> data : ");
  print_bits_of_byte(payload_cobs->data_ptr[index], true);
  RADIO_LOG_OUTPUT("[payload]  -> length: %d\n", payload_cobs[index].data_len);
}

static void log_cobs_encoded(uint8_t *encoded_payload_byte_ptr,
                             cobs_encode_result encoded_result) {
  RADIO_LOG_OUTPUT("[encoded]  -> data : %u \n",
                   *(encoded_payload_byte_ptr));
  // fixme : wrong value data
  RADIO_LOG_OUTPUT("[encoded]  -> data : ");
  print_bits_of_byte(*(encoded_payload_byte_ptr), true);
  RADIO_LOG_OUTPUT("[encoded]  -> length: %d\n", encoded_result.out_len);
}

static void log_cobs_decoded(uint8_t *decoded_payload_byte_ptr,
                             cobs_decode_result decoded_result) {
  // fixme : wrong value data
  RADIO_LOG_OUTPUT("[decoded]  -> data : %u \n",
                   *(decoded_payload_byte_ptr));
  RADIO_LOG_OUTPUT("[decoded]  -> data : ");
  print_bits_of_byte(*(decoded_payload_byte_ptr), true);
  RADIO_LOG_OUTPUT("[decoded]  -> length: %d\n", decoded_result.out_len);
}

/**
 * @brief Example to en-, decode some test data.
 *
 */
void radio_encoding_cobs_example(void) {
  cobs_data cobs_data_tests[] = {
      {"", 0},
      {"1", 1},
      {"26.44", 5},
      {"\x00"
       "12345\x00"
       "6789",
       11},
      {"\x00", 1},
      {"\x00\x00", 2},
      {"\x00\x00\x00", 3},
  };
  uint8_t encode_out[COBS_ENCODE_DST_BUF_LEN_MAX(100)];
  cobs_encode_result encode_result;
  uint8_t decode_out[100];
  cobs_decode_result decode_payload;
  size_t i;

  for (i = 0; i < dimof(cobs_data_tests); i++) {
    // memset(encode_out, 'A', sizeof(encode_out));
    // -- encode
    encode_result =
        cobs_encode(encode_out, sizeof(encode_out), cobs_data_tests[i].data_ptr,
                    cobs_data_tests[i].data_len);
    printf("encode data: %s\n", cobs_data_tests[i].data_ptr);
    print_bits_of_byte(*(cobs_data_tests[i].data_ptr), true);
    printf("encode data len: %d\n", cobs_data_tests[i].data_len);

    // -- decode
    decode_payload = cobs_decode(decode_out, sizeof(decode_out), encode_out,
                                 encode_result.out_len);
    printf("decode data: %u\n", decode_out[i]);
    print_bits_of_byte(decode_out[i], true);
    printf("decode data len: %d\n", decode_payload.out_len);
  }
}