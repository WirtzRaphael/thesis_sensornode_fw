#ifndef RADIO_H_
#define RADIO_H_

#include <errno.h>
#include <stdint.h>
#include "sensors.h"
#include "cobs.h"

typedef struct {
  uint8_t *data_ptr;
  size_t data_len;
} cobs_data;

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
void radio_send_temperature_as_string(temperature_measurement_t *temperature_measurement, bool dryrun);
void radio_send_temperature_as_bytes(temperature_measurement_t *temperature_measurement, bool dryrun);
void radio_send_test(void);
char radio_get_rf_destination_address(void);
static void print_bits_of_byte(uint8_t byte, bool print);

#endif /* RADIO_H_ */