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

// todo [demo] : replace mcutimedate library ?
uint32_t alert_time_unix = 0;
DATEREC alert_date = {0, 0, 0};
TIMEREC alert_time = {0, 0, 0, 0};
DATEREC date = {2024, 7, 1};
TIMEREC time = {0, 0, 0, 0};
uint32_t time_unix = 0;
uint8_t offset_hours = 0;

static bool enable = true;
static bool disabled = false;

void time_rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second) {
  uint8_t sec100 = 0;
  ExtRTC_SetTimeInfo(hour, minute, second, sec100);
}

/**
 * @brief Get current time
 *
 * @param time
 * @return error_t
 */
error_t time_rtc_get_time(TIMEREC *time) {
  if (ExtRTC_GetTime(time) == ERR_OK) {
    return ERR_OK;
  } else {
    return ERR_FAILED;
  }
}

/**
 * @brief Get current date
 *
 * @param date
 * @return error_t
 */
error_t time_rtc_get_date(DATEREC *date) {
  if (ExtRTC_GetDate(date) == ERR_OK) {
    return ERR_OK;
  } else {
    return ERR_FAILED;
  }
}

/**
 * @brief Set date
 *
 * @param datevar
 * @return error_t
 */
error_t time_rtc_set_date(DATEREC *datevar) {
  if (ExtRTC_SetDateInfo(datevar->Year, datevar->Month, datevar->Day) ==
      ERR_OK) {
    return ERR_OK;
  } else {
    return ERR_FAILED;
  }
}

/**
 * @brief rtc software reset
 *
 * @return error_t
 */
error_t time_rtc_software_reset(void) {
  return McuPCF85063A_WriteSoftwareReset();
}

/**
 * @brief Set alarm from now in seconds
 *
 * @param t_from_now_s
 * @return error_t
 */
error_t time_rtc_alarm_from_now_s(uint16_t *t_from_now_s) {
  time_rtc_get_time(&time);
  time_rtc_get_date(&date);
  // unix time conversion to avoid overflow handling for seconds, minutes, hour
  time_unix = McuTimeDate_TimeDateToUnixSeconds(&time, &date, offset_hours);
  alert_time_unix = time_unix + 5;
  // convert unix time to alert time and date
  McuTimeDate_UnixSecondsToTimeDate(alert_time_unix, offset_hours, &alert_time,
                                    &alert_date);

  time_rtc_alarm_set_time(&alert_time, &alert_date);
}

error_t time_rtc_alarm_set_time(TIMEREC *alert_time, DATEREC *alert_date) {
  uint8_t val = 0;
  bool dummy;
  bool is24h, isAM;

  // write alarm time
  if (McuPCF85063A_ReadAlarmSecond(&val, &dummy) != ERR_OK) {
    McuLog_error("Error reading alarm second\n");
    return ERR_FAILED;
  }
  McuPCF85063A_WriteAlarmSecond(alert_time->Sec, enable);
  McuLog_trace("Alarm sec : %d\n", alert_time->Sec);
  if (McuPCF85063A_ReadAlarmMinute(&val, &dummy) != ERR_OK) {
    McuLog_error("Error reading alarm minute\n");
    return ERR_FAILED;
  }
  McuPCF85063A_WriteAlarmMinute(alert_time->Min, enable);
  McuLog_trace("Alarm min : %d\n", alert_time->Min);
  if (McuPCF85063A_ReadAlarmHour(&val, &dummy, &is24h, &isAM) != ERR_OK) {
    McuLog_error("Error reading alarm hour\n");
    return ERR_FAILED;
  }
  McuPCF85063A_WriteAlarmHour(alert_time->Hour, enable, is24h, isAM);
  McuLog_trace("Alarm hour : %d\n", alert_time->Hour);

  return ERR_OK;
}

/**
 * @brief Check if alarm time is in the future
 *
 * @return uint8_t
 */
uint8_t time_rtc_alarm_check_future(void) {
  alert_enabled alert_enabled;
  time_rtc_get_time(&time);
  time_unix = McuTimeDate_TimeDateToUnixSeconds(&time, &date, offset_hours);
  //
  time_rtc_alarm_get_time(&alert_time, &alert_enabled);
  alert_time_unix =
      McuTimeDate_TimeDateToUnixSeconds(&alert_time, &alert_date, offset_hours);

  if (time_unix < alert_time_unix) {
    return ERR_OK;
  } else {
    return ERR_FAILED;
  }
}

/**
 * @brief Get alarm time
 *
 * @param time
 * @param alert_enabled
 * @return uint8_t
 */
uint8_t time_rtc_alarm_get_time(TIMEREC *time, alert_enabled *alert_enabled) {
  uint8_t val = 0;
  bool dummy;
  bool is24h, isAM;

  time->Sec100 = 0; // hint : not supported

  if (McuPCF85063A_ReadAlarmSecond(&val, &dummy) == ERR_OK) {
    time->Sec = val;
    alert_enabled->sec_enabled = dummy;
  }
  if (McuPCF85063A_ReadAlarmMinute(&val, &dummy) == ERR_OK) {
    time->Min = val;
    alert_enabled->min_enabled = dummy;
  }
  if (McuPCF85063A_ReadAlarmHour(&val, &dummy, &is24h, &isAM) == ERR_OK) {
    time->Hour = val;
    alert_enabled->hour_enabled = dummy;
  }
  return ERR_OK;
}

/**
 * @brief Enable alarm
 *
 * @return error_t
 */
error_t time_rtc_alarm_enable(void) {
  McuLog_info("Alarm ");
  return McuPCF85063A_WriteAlarmInterrupt(enable);
}

/**
 * @brief Reset alarm flag
 *
 * @return error_t
 */
error_t time_rtc_alarm_reset_flag(void) {
  return McuPCF85063A_WriteResetAlarmInterrupt();
}

#endif