/**
 * @file menu.c
 * @author Raphael Wirtz
 * @brief Display a menu to execute commands of the system
 * @date 2024-05-12
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "menu.h"
#include "extRTC.h"
#include "pico_config.h"
#include "power.h"
#include "radio.h"
#include "rc232.h"
#include "sensors.h"
#include "time.h"
#include "stdio.h"
#include <stdint.h>

#include "McuLog.h"

#include "pico/stdlib.h"
// todo : remove

#ifndef dimof
  #define dimof(X) (sizeof(X) / sizeof((X)[0]))
#endif

#if PICO_CONFIG_USE_RTC
// DATEREC date_rtc_menu;
// TIMEREC time_rtc_menu;
DATEREC date_rtc_ext_menu;
DATEREC date_rtc_ext_default_menu;
TIMEREC time_rtc_ext_menu;
TIMEREC time_rtc_ext_default_menu;

void menu_reset_time(void) {
  time_rtc_ext_default_menu.Hour = 1;
  time_rtc_ext_default_menu.Min = 0;
  time_rtc_ext_default_menu.Sec = 0;
  time_rtc_ext_default_menu.Sec100 = 0;
}

void menu_set_time_default(void) {
  time_rtc_ext_default_menu.Hour = 6;
  time_rtc_ext_default_menu.Min = 10;
  time_rtc_ext_default_menu.Sec = 0;
  time_rtc_ext_default_menu.Sec100 = 0;
}

void menu_reset_time_date(void) {
  // date - example
  date_rtc_ext_default_menu.Day = 1;
  date_rtc_ext_default_menu.Month = 1;
  date_rtc_ext_default_menu.Year = 2000;
}

void menu_set_date_default(void) {
  date_rtc_ext_default_menu.Day = 19;
  date_rtc_ext_default_menu.Month = 5;
  date_rtc_ext_default_menu.Year = 2024;
}

// todo : move function into another file like time/power/...
// todo : extend to wakeup for measurement time
// i.e. wakeup every 10 seconds (10:00:00, 10:00:10,...)
// note: alarm time means not exactly time X from now, because of the current
// time accuracy. e.g. wakeup 5 seconds from now -> 10:00:04:500 -> (+4.5) ->
// 10:00:09:000
static void menu_alarm_set_time(void) {
  time_rtc_alarm_set_time();
}

// todo : move function into another file like time/power/...
void menu_read_alarm(void) {
  time_rtc_alarm_get_time();
}
#endif

/**
 * @brief Task for the menu
 *
 * @param pvParameters
 */
static void vMenuTask(void *pvParameters) {

  for (;;) {
    menu_handler_main();
  }
}

/**
 * @brief Initialize the menu task
 *
 */
void menu_init(void) {
  if (xTaskCreate(vMenuTask, /* pointer to the task */
                  "menu",    /* task name for kernel awareness debugging */
                  1000 / sizeof(StackType_t), /* task stack size */
                  (void *)NULL,         /* optional task startup argument */
                  tskIDLE_PRIORITY + 2, /* initial priority */
                  (TaskHandle_t *)NULL  /* optional task handle to create */
                  ) != pdPASS) {
    for (;;) {
    } /* error! probably out of memory */
  }
}

/**
 * @brief Display the menu options
 *
 * @param options
 * @param numOptions
 */
void menu_display(const char *options[], int numOptions) {
  for (int i = 0; i < numOptions; i++) {
    printf("%s \n", options[i]);
  }
  printf("Enter character: \n");
}

/**
 * @brief Get the User Input for the menu
 *
 * @return char
 */
char menu_get_user_input() {
  char userCmd = getchar();
  printf("You entered: %c\n\n", userCmd);
  return userCmd;
}

/**
 * @brief menu for main features
 *
 */
void menu_handler_main(void) {
  const char *mainMenuOptions[] = {
      "[r]adio", "rc[2]32",   "rc232 [c]onfiguration",
      "[p]ower", "[s]ensors", "[t]ime"};
  menu_display(mainMenuOptions, dimof(mainMenuOptions));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'r':
#if PICO_CONFIG_USE_RADIO
    menu_handler_radio();
#endif
    break;
  case '2':
#if PICO_CONFIG_USE_RADIO
    menu_handler_rc232();
#endif
    break;
  case 'c':
#if PICO_CONFIG_USE_RADIO
    menu_handler_rc232_config();
#endif
    break;
  case 's':
#if PICO_CONFIG_USE_SENSORS
    menu_handler_sensors();
#endif
    break;
  case 't':
#if PICO_CONFIG_USE_RTC
    menu_handler_time();
#endif
    break;
  case 'p':
#if PICO_CONFIG_USE_POWER
    menu_handler_power();
#endif
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}

#if PICO_CONFIG_USE_RADIO
/**
 * @brief menu for radio features
 *
 */
void menu_handler_radio(void) {
  const char *radioOptions[] = {"[a]uthenticate", "encoding [c]obs", "[s]end",
                                "send [t]est message"};
  menu_display(radioOptions, dimof(radioOptions));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'a':
    radio_authentication();
    break;
  case 'c':
    radio_encoding_hdlc_example();
    break;
  case 's':
    // radio_send_temperature_as_string();
    break;
  case 't':
    radio_send_test_string();
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}

/**
 * @brief menu for rc232 features
 *
 */
void menu_handler_rc232(void) {
  const char *rc232Options[] = {"[b]uffer read out / receive",
                                "broad[c]ast",
                                "[r]eceive",
                                "[s]end",
                                "[t]emperature",
                                "[v]oltage",
                                "s[l]eep",
                                "[w]ake up"};
  menu_display(rc232Options, dimof(rc232Options));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'b':
    rc232_rx_read_buffer_full();
    break;
  case 'c':
    rc232_config_destination_address(RC232_BROADCAST_ADDRESS);
    radio_send_test_string();
    rc232_config_destination_address(
        radio_get_rf_destination_address()); // set back to default
    break;
  case 'l':
    rc232_sleep();
    break;
  case 'r':
    rc232_rx_read_buffer_full(); // same as buffer read out
    break;
  case 's':
    rc232_tx_packet_string("RC232 Send String", false);
    break;
  case 't':
    rc232_read_temperature();
    break;
  case 'v':
    rc232_read_voltage();
    break;
  case 'w':
    rc232_wakeup();
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}

/**
 * @brief
 *
 */
void menu_handler_rc232_config(void) {
  const char *rc232Options[] = {"[c]hannel",
                                "[d]estination address",
                                "rss[i]",
                                "memory read [b]yte (NVM)",
                                "memory [w]rite (NVM)",
                                "[m]emory read config",
                                "[p]ower",
                                "[r]eset",
                                "e[x]it config state",
                                "[0] get config / check if config mode"};
  menu_display(rc232Options, dimof(rc232Options));

  char userCmd = menu_get_user_input();
  uint8_t rssi;
  switch (userCmd) {
  case 'c':
    rc232_config_rf_channel_number(5);
    break;
  case 'd':
    rc232_config_destination_address(20);
    break;
  case 'i':
    rssi = rc232_signal_strength_indicator();
    break;
  case 'b':
    rc232_memory_read_one_byte(NVM_ADDR_RF_CHANNEL);
    rc232_memory_read_one_byte(NVM_ADDR_RF_POWER);
    rc232_memory_read_one_byte(NVM_ADDR_RF_DATA_RATE);
    break;
  case 'm':
    rc232_memory_read_configuration();
    break;
  case 'p':
    rc232_config_rf_power(1);
    break;
  case 'w':
    rc232_memory_write_configuration();
    break;
  case 'r':
    rc232_reset();
    break;
  case 'x':
    exit_config_state();
    break;
  case '0':
    rc232_get_configuration_memory();
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}
#endif

#if PICO_CONFIG_USE_SENSORS
void menu_handler_sensors(void) {
  const char *sensorsOptions[] = {"[r]ead temperature (queue latest)"};
  menu_display(sensorsOptions, dimof(sensorsOptions));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'r':
    // fixme : not shared access to queue (i.e. different task)
    // sensors_print_temperatures_queue_peak();
    sensors_print_temperature_xQueue_latest_all();
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}
#endif

#if PICO_CONFIG_USE_RTC
// todo : capacitor configuration (7pF)
// todo : turn on int pin
// todo : check alarm flag
// (https://tronixstuff.com/2013/08/13/tutorial-arduino-and-pcf8563-real-time-clock-ic/)
void menu_handler_time(void) {
  const char *timeOptions[] = {"rtc time : Get [t]ime",
                               "rtc time : Get [d]ate",
                               "rtc time : [1] Set time",
                               "rtc time : [2] Set date",
                               "\n",
                               "rtc alarm : Enable [a]larm interrupt",
                               "rtc alarm : [r]eset alarm interrupt",
                               "rtc alarm : [3] Set alarm time",
                               "rtc alarm : [4] Get alarm time",
                               "\n",
                               "rtc : [9] Software Reset",
                               "rtc : Clock [o]utput frequency"};
  menu_display(timeOptions, dimof(timeOptions));

  uint8_t ret_time = 0;

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'a':
    time_rtc_alarm_enable();
    printf("Alarm interrupt enabled.\n");
    printf("Set alarm interrupt (AIE) bit\n");
    break;
  case 'r':
    time_rtc_alarm_reset_flag();
    printf("Reset alarm interrupt\n");
    printf("Reset alarm flag (AF) bit\n");
    break;
  case '3':
    menu_alarm_set_time();
    break;
  case '4':
    menu_read_alarm();
    break;
  case 'd':
    menu_reset_time_date();
    ret_time = ExtRTC_GetDate(&date_rtc_ext_menu);
    // McuExtRTC_GetDate(&date_menu)
    // McuExtRTC_GetRTCDate(McuExtRTC_TDATE *date)
    printf("Date: %d.%d.%d\n", date_rtc_ext_menu.Day, date_rtc_ext_menu.Month,
           date_rtc_ext_menu.Year);
    break;
  case 't':
    menu_reset_time();
    ret_time = ExtRTC_GetTime(&time_rtc_ext_menu);
    // McuExtRTC_GetTime(&time_menu)
    // McuExtRTC_GetRTCTime(McuExtRTC_TTIME * time)
    printf("Time: %d:%d:%d\n", time_rtc_ext_menu.Hour, time_rtc_ext_menu.Min,
           time_rtc_ext_menu.Sec);
    break;
  case '1':
    menu_set_time_default();
    ret_time = ExtRTC_SetTimeInfo(
        time_rtc_ext_default_menu.Hour, time_rtc_ext_default_menu.Min,
        time_rtc_ext_default_menu.Sec, time_rtc_ext_default_menu.Sec100);
    // McuExtRTC_SetTime(uint8_t Hour, uint8_t Min, uint8_t Sec, uint8_t Sec100)
    // McuExtRTC_SetRTCTime(&time_rtc_ext_default_menu);
    printf("Set time: %d:%d:%d\n", time_rtc_ext_default_menu.Hour,
           time_rtc_ext_default_menu.Min, time_rtc_ext_default_menu.Sec);
    break;
  case '2':
    menu_set_date_default();
    ret_time = ExtRTC_SetDateInfo(date_rtc_ext_default_menu.Year,
                                  date_rtc_ext_default_menu.Month,
                                  date_rtc_ext_default_menu.Day);
    // McuExtRTC_SetDate(uint16_t Year, uint8_t Month, uint8_t Day) break;
    // McuExtRTC_SetRTCDate(McuExtRTC_TDATE *date);
    printf("Set date: %d.%d.%d\n", date_rtc_ext_default_menu.Day,
           date_rtc_ext_default_menu.Month, date_rtc_ext_default_menu.Year);
    break;
  case '9':
    ret_time = time_rtc_software_reset();
    printf("Software reset\n");
    break;
  default:
    printf("Invalid option\n");
    break;
  }
  printf("return of time functions: %d\n", ret_time);
}
#endif

#if PICO_CONFIG_USE_POWER
void menu_handler_power(void) {
  const char *sensorsOptions[] = {"[1] start VCC-1", "[2] shtudown VCC-1",
                                  "power mode [l]ight", "power mode [h]eavy"};
  menu_display(sensorsOptions, dimof(sensorsOptions));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case '1':
    power_3v3_1_enable(true);
    break;
  case '2':
    power_3v3_1_enable(false);
    break;
  case 'l':
    power_mode(POWER_MODE_LIGHT);
    break;
  case 'h':
    power_mode(POWER_MODE_HEAVY);
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}
#endif