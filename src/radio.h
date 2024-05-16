#ifndef RADIO_H_
#define RADIO_H_

#include "pico/types.h"
#include "sensors.h"
#include <errno.h>
#include <stdint.h>

#define RADIO_BUFFER_TEMPERATURE_LEN(SRC_LEN) (((SRC_LEN) * 2) + 1)
#define RADIO_TEMPERATURE_VALUES              5
#define RADIO_DEBUG_DECODE                    (1)

typedef enum {
  AUTHENTICATION,
  SENSORS_TEMPERATURE_1,
  SENSORS_TEMPERATURE_2,
  SENSORS_TEMPERATURE_3,
  SENSORS_TEMPERATURE_4,
  SENSORS_RS232_1,
} data_content;

typedef struct {
  uint8_t protocol_version : 3;
  data_content data_content : 5;
} data_info_field;

typedef struct {
  data_info_field data_info;
  temperature_measurement_t measurement;
  uint8_t measurement_byte[2];
  uint8_t index;
  uint8_t temperature_index;
  // uint8_t start_index;
  // uint8_t end_index;
  // uint8_t src_index;
} radio_data_temperature_t;

typedef struct {
  // volatile
  uint8_t destination_address;
  uint8_t source_address;
  // fix values
  uint8_t channel_id;
  uint8_t channel_default;
  uint8_t channel_start;
  uint8_t channel_end;
} rf_settings_t;

static void
convert_temperature_to_byte(uint8_t *data_16LE_byte,
                            temperature_measurement_t *temperature_measurement);
static void log_hdlc_data(char *data_ptr, size_t send_data);
static void log_hdlc_encoded(char *encoded_ptr, size_t encoded_len);
static void log_hdlc_decoded(char *decoded_ptr, size_t decoded_len);
static void log_buffer_as_char(char *buffer, size_t length);
static void log_buffer_as_int(char *buffer, size_t length);
void radio_authentication(void);
void radio_encoding_hdlc_example(void);
static void radio_send_authentication_request(void);
static error_t radio_wait_for_authentication_response(uint32_t timeout_ms);
void radio_init(void);
void radio_send_temperature_as_string(
    temperature_measurement_t *temperature_measurement, bool dryrun);
error_t radio_send_temperature_as_bytes(QueueHandle_t xQueue_temperature,
                                        data_info_field data_info_field,
                                        bool dryrun);
void radio_send_test_string(void);
char radio_get_rf_destination_address(void);
static void print_bits_of_byte(uint8_t byte, bool print);

#endif /* RADIO_H_ */