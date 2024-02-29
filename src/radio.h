#ifndef RADIO_H_
#define RADIO_H_

#include "sensors.h"

#define STRING_LENGTH_UINT8 ((CHAR_BIT * sizeof(uint8_t) - 1) / 3 + 2)

void radio_send_test_messages(void);
void radio_send_sensor_temperature(queue_t *temperatureSensor1_queue, queue_t *temperatureSensor2_queue);
void radio_send_sensor_temperature_series(time_series_sensor_t time_series_sensor);

#endif /* RADIO_H_ */