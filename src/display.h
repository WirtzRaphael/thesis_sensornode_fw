#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "pico/stdlib.h"

void display_init(void);
void Show1Liner(const unsigned char *text0);
void Show2Liner(const unsigned char *text0, const unsigned char *text1);
void display_status_float(float temperature1, float temperature2);
void display_status_uint16(uint16_t sensor1, uint16_t sensor2);

#endif /* DISPLAY_H_ */