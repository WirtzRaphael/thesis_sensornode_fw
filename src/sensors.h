#ifndef SENSORS_H_
#define SENSORS_H_

#include "hardware/i2c.h"
#include "pico/util/queue.h"
#include <errno.h>
#include <stdint.h>

typedef struct {
  i2c_inst_t *i2c;
  uint8_t i2c_address;
  uint8_t sensor_nr;
  uint64_t start_measurement_time;
  queue_t *measurments_queue;
} temperature_sensor_t;

typedef struct {
  float temperature;
  uint8_t id;
  uint16_t timediff_to_start;
} temperature_measurement_t;

typedef struct {
  char sensor_nr[1];
  uint64_t time_reference;
  queue_t *queue;
} time_series_sensor_t;

void sensors_init(void);
error_t
sensors_read_temperature(temperature_sensor_t *temperature_sensor,
                         temperature_measurement_t *temperature_measurement);
static error_t add_temperature_to_queue(queue_t *temperature_sensor_queue,
                                 temperature_measurement_t *temperature);
// todo : rename functions
error_t sensors_get_latest_temperature(queue_t *temperature_sensor_queue,
                               float *temperature);
uint16_t sensors_get_sampling_time(void);
void sensors_print_temperatures_queue_peak(void);
static void print_temperature_queue_peak(queue_t *temperature_sensor_queue);

#endif // SENSORS_H_