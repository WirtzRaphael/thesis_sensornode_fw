#include <stdio.h>
#include "sensors.h"

void reset_temperature_entry(sensor_temp_t *sensor) {
    sensor->temperature = 0.0f;
    sensor->id = 0;
    sensor->time_relative_to_reference = 0;
}

float get_latest_temperature(queue_t temperature_sensor_queue) {
    sensor_temp_t temperature_entry;
    if (queue_try_peek(&temperature_sensor_queue, &temperature_entry)) {
        return temperature_entry.temperature;
    }
    else {
        return 0.0f;
    }
}

void print_sensor_temperatures(queue_t temperature_sensor_queue) {
    sensor_temp_t temperature_entry;
    if (queue_try_peek(&temperature_sensor_queue, &temperature_entry)) {
        printf("QUEUE peek success\r\n");
        printf("QUEUE peek temperature: %f\r\n", temperature_entry.temperature);
        printf("QUEUE peek id: %d\r\n", temperature_entry.id);
        printf("QUEUE peek time relative: %d\r\n", temperature_entry.time_relative_to_reference);
    }
    
    /*
    if (queue_try_remove(&temperatureSensor1_queue, &temperature_entry)) {
        printf("QUEUE remove success\r\n");
        printf("QUEUE remove temperature: %f\r\n", temperature_entry.temperature);
        printf("QUEUE remove id: %d\r\n", temperature_entry.id);
    }
    */
}
