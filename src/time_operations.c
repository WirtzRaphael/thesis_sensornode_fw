#include "platform_config.h"

#if PLATFORM_CONFIG_USE_RTC
  #include "McuLib.h"
  #include "McuLog.h"
  #include "McuTimeDate.h"
  #include "stdio.h"
  #include "time_operations.h"
  #include <errno.h>

  #include "McuPCF85063A.h"
  #include "extRTC.h"

DATEREC date;
TIMEREC time;

static bool enable = true;
static bool disabled = false;

void time_rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second) {
  time.Hour = hour;
  time.Min = minute;
  time.Sec = second;
  time.Sec100 = 0;
}

// Get current time
error_t time_rtc_get_time(TIMEREC *time) {
  if (ExtRTC_GetTime(time) == ERR_OK) {
    return ERR_OK;
  } else {
    return ERR_FAILED;
  }
}

// Get current date
error_t time_rtc_get_date(DATEREC *date) {
  if (ExtRTC_GetDate(date) == ERR_OK) {
    return ERR_OK;
  } else {
    return ERR_FAILED;
  }
}

error_t time_rtc_software_reset(void) {
  return McuPCF85063A_WriteSoftwareReset();
}

error_t time_rtc_alarm_from_now_s(uint16_t *t_from_now_s) {
  uint32_t alert_unix_time = 0;
  uint8_t val = 0;
  DATEREC alert_date = {0, 0, 0};
  TIMEREC alert_time = {0, 0, 0, 0};
  bool dummy;
  bool is24h, isAM;

  time_rtc_get_time(&time);
  time_rtc_get_date(&date);

  unix_time = McuTimeDate_TimeDateToUnixSeconds(&time, &date, offset_hours);
  alert_unix_time = unix_time + 5;
  McuTimeDate_UnixSecondsToTimeDate(alert_unix_time, offset_hours, &alert_time,
                                    &alert_date);

  if (McuPCF85063A_ReadAlarmSecond(&val, &dummy) != ERR_OK) {
    McuLog_error("Error reading alarm second\n");
    return ERR_FAILED;
  }
  McuPCF85063A_WriteAlarmSecond(alert_time.Sec, enable);
  McuLog_trace("Alarm sec : %d\n", alert_time.Sec);
  if (McuPCF85063A_ReadAlarmMinute(&val, &dummy) != ERR_OK) {
    McuLog_error("Error reading alarm minute\n");
    return ERR_FAILED;
  }
  McuPCF85063A_WriteAlarmMinute(alert_time.Min, enable);
  McuLog_trace("Alarm min : %d\n", alert_time.Min);
  if (McuPCF85063A_ReadAlarmHour(&val, &dummy, &is24h, &isAM) != ERR_OK) {
    McuLog_error("Error reading alarm hour\n");
    return ERR_FAILED;
  }
  McuPCF85063A_WriteAlarmHour(alert_time.Hour, enable, is24h, isAM);
  McuLog_trace("Alarm hour : %d\n", alert_time.Hour);

  return ERR_OK;
}

}

void time_rtc_alarm_set_time(void) {
  // todo : params
  // alarm time from current time
  uint8_t alarm_h = 0;
  uint8_t alarm_m = 0;
  uint8_t alarm_s = 5;

  uint8_t ret_time = 0;
  uint8_t val = 0;
  bool dummy;
  bool is24h, isAM;

  time_rtc_get_time(&time);

  if (McuPCF85063A_ReadAlarmSecond(&val, &dummy) != ERR_OK) {
    printf("Error reading alarm second\n");
    return;
  }
  McuPCF85063A_WriteAlarmSecond((time.Sec + alarm_s), enable);
  printf("Alarm sec : %d\n", alarm_s);
  if (McuPCF85063A_ReadAlarmMinute(&val, &dummy) != ERR_OK) {
    printf("Error reading alarm minute\n");
    return;
  }
  McuPCF85063A_WriteAlarmMinute((time.Min + alarm_m), enable);
  printf("Alarm min : %d\n", alarm_m);
  if (McuPCF85063A_ReadAlarmHour(&val, &dummy, &is24h, &isAM) != ERR_OK) {
    printf("Error reading alarm hour\n");
    return;
  }
  McuPCF85063A_WriteAlarmHour((time.Hour + alarm_h), enable, is24h, isAM);
  printf("Alarm hour : %d\n", alarm_h);
}

void time_rtc_alarm_get_time(void) {
  uint8_t val = 0;
  bool dummy;
  bool is24h, isAM;

  if (McuPCF85063A_ReadAlarmSecond(&val, &dummy) == ERR_OK) {
    printf("Alarm sec : %d\n", val);
    printf("- Enabled : %d\n", dummy);
  }
  if (McuPCF85063A_ReadAlarmMinute(&val, &dummy) == ERR_OK) {
    printf("Alarm min : %d\n", val);
    printf("- Enabled : %d\n", dummy);
  }
  if (McuPCF85063A_ReadAlarmHour(&val, &dummy, &is24h, &isAM) == ERR_OK) {
    printf("Alarm hour : %d\n", val);
    printf("- Enabled : %d\n", dummy);
  }
  if (McuPCF85063A_ReadAlarmDay(&val, &dummy) == ERR_OK) {
    printf("Alarm day : %d\n", val);
    printf("- Enabled : %d\n", dummy);
  }
  if (McuPCF85063A_ReadAlarmWeekday(&val, &dummy) == ERR_OK) {
    printf("Alarm weekday : %d\n", val);
    printf("- Enabled : %d\n", dummy);
  }
}

error_t time_rtc_alarm_enable(void) {
  McuLog_info("Alarm ");
  return McuPCF85063A_WriteAlarmInterrupt(enable);
}

error_t time_rtc_alarm_reset_flag(void) {
  return McuPCF85063A_WriteResetAlarmInterrupt();
}

#endif