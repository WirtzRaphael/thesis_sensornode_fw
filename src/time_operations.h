#ifndef TIME_H_
#define TIME_H_

#include "pico/stdlib.h"
#include "McuTimeDate.h"
#include <errno.h>

void time_rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second);
error_t time_rtc_software_reset(void);
void time_rtc_alarm_set_time(void);
error_t time_rtc_alarm_from_now(TIMEREC *t_from_now);
void time_rtc_alarm_get_time(void);
error_t time_rtc_alarm_enable(void);
error_t time_rtc_alarm_reset_flag(void);

#endif /* TIME_H_ */