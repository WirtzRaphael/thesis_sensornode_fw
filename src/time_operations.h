#ifndef TIME_H_
#define TIME_H_

#include "pico/stdlib.h"
#include "McuTimeDate.h"
#include <errno.h>
typedef struct {
  bool sec100_enabled;
  bool sec_enabled;
  bool min_enabled;
  bool hour_enabled;
} alert_enabled;

void time_rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second);
error_t time_rtc_software_reset(void);
uint8_t time_rtc_alarm_check_future(void);
error_t time_rtc_alarm_from_now_s(uint16_t *t_from_now_s);
uint8_t time_rtc_alarm_get_time(TIMEREC *time, alert_enabled *alert_enabled);
error_t time_rtc_alarm_enable(void);
error_t time_rtc_alarm_reset_flag(void);

#endif /* TIME_H_ */