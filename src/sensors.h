#ifndef SENSORS_H
#define SENSORS_H

#include "pico/stdlib.h"
#include "pico/util/queue.h"

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

void reset_temperature_entry(sensor_temp_t *sensor);
float get_latest_temperature(queue_t temperature_sensor_queue);

void print_sensor_temperatures(queue_t temperature_sensor_queue);

#endif /* SENSORS_H */