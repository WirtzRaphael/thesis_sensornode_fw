#ifndef SENSORS_H_
#define SENSORS_H_

#include "pico/util/queue.h"
#include "hardware/i2c.h"

typedef struct {
    float temperature;
    uint8_t id;
    uint16_t time_relative_to_reference;
}sensor_temp_t;

typedef struct { 
    char sensor_nr[1];
    uint64_t time_reference;
    queue_t *queue;
}time_series_sensor_t;

void sensors_init(void);
void sensors_read_temperature(i2c_inst_t *i2c);
// todo : rename functions
float get_latest_temperature(queue_t temperature_sensor_queue);
void print_sensor_temperatures(queue_t temperature_sensor_queue);

#endif // SENSORS_H_