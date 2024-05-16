#ifndef RADIO_H_
#define RADIO_H_

#include "sensors.h"
#include <errno.h>
#include <stdint.h>

static void convert_temperature_to_byte(uint8_t *data_16LE_byte,
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
error_t radio_send_temperature_as_bytes(
    temperature_measurement_t *temperature_measurement, bool dryrun);
void radio_send_test_string(void);
char radio_get_rf_destination_address(void);
static void print_bits_of_byte(uint8_t byte, bool print);

#endif /* RADIO_H_ */