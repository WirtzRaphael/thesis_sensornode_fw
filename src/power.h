#ifndef POWER_H_
#define POWER_H_

#include <errno.h>
#include "pico/stdlib.h"

typedef enum {
  POWER_MODE_LIGHT,
  POWER_MODE_HEAVY
}power_mode_t;

void power_init(void);
void power_3v3_1_enable(bool enable);
void power_3v3_2_enable(bool enable);
error_t power_mode(power_mode_t mode);

#endif /* POWER_H_ */