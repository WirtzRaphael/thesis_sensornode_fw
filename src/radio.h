#ifndef RADIO_H_
#define RADIO_H_

#include "cobs.h"
#include "sensors.h"
#include <errno.h>
#include <stdint.h>

static void convert_temperature_to_byte(uint8_t *data_16LE_byte,
                                 temperature_measurement_t *temperature_measurement);
static void log_cobs_encoded(uint8_t *encoded_payload_byte_ptr,
                             cobs_encode_result encoded_result);
static void log_cobs_decoded(uint8_t *decoded_payload_byte_ptr,
                             cobs_decode_result decoded_result);
static void log_cobs_payload(uint8_t *payload_byte_ptr, size_t length);
static void log_print_buffer_as_char(uint8_t *buffer, size_t length);
void radio_authentication(void);
void radio_encoding_cobs_example(void);
static void radio_send_authentication_request(void);
static error_t radio_wait_for_authentication_response(uint32_t timeout_ms);
/*
cobs_encode_result radio_encode(void *encoded_payload_ptr,
                                size_t encoded_payload_len,
                                cobs_data *payload_bytes,
                                size_t payload_bytes_len);
                                */
void radio_init(void);
void radio_send_temperature_as_string(
    temperature_measurement_t *temperature_measurement, bool dryrun);
void radio_send_temperature_as_bytes(
    temperature_measurement_t *temperature_measurement, bool dryrun);
void radio_send_temperature_as_bytes_cobs(
    temperature_measurement_t *temperature_measurement, bool dryrun);
void radio_send_test(void);
char radio_get_rf_destination_address(void);
static void print_bits_of_byte(uint8_t byte, bool print);

#endif /* RADIO_H_ */