#ifndef POWER_H_
#define POWER_H_

#include <errno.h>
#include "pico/stdlib.h"

typedef enum {
  POWER_MODE_LIGHT,
  POWER_MODE_HEAVY
}power_3V3_mode_t;

typedef enum {
  POWER_STATE_ON,
  POWER_STATE_OFF,
  POWER_STATE_SLEEP
}power_state_t;


void power_init(void);
void power_3v3_1_enable(bool enable);
void power_3v3_2_enable(bool enable);

// todo : sleep radio (wrapper), use power_state_t
// todo : sleep mcu (sdk pico-extra)
// todo : rtc function (wrapper) ?
error_t power_3V3_mode(power_3V3_mode_t mode);

bool power_get_periodic_shutdown(void);
void power_set_periodic_shutdown(bool shutdown);
void power_toggle_periodic_shutdown(void);

#endif /* POWER_H_ */