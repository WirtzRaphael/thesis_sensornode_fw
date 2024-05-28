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

bool power_get_periodic_shutdown(void);
void power_set_periodic_shutdown(bool shutdown);

#endif /* POWER_H_ */