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
#include "radio.h"
#include "rc232.h"
#include "sensors.h"
#include "stdio.h"
#include <stdint.h>

#include "pico/stdlib.h"

DATEREC date_menu;
TIMEREC time_menu;

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
      "[r]adio", "rc[2]32", "rc232 [c]onfiguration", "[s]ensors", "[t]ime"};
  menu_display(mainMenuOptions, dimof(mainMenuOptions));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'r':
    menu_handler_radio();
    break;
  case '2':
    menu_handler_rc232();
    break;
  case 'c':
    menu_handler_rc232_config();
    break;
  case 's':
    menu_handler_sensors();
    break;
  case 't':
    menu_handler_time();
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}

/**
 * @brief menu for radio features
 *
 */
void menu_handler_radio(void) {
  const char *radioOptions[] = {"[a]uthenticate", "encoding [c]obs", "[s]end",
                                "send [t]est message"};
  menu_display(radioOptions, 4);

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
  menu_display(rc232Options, 8);

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
  menu_display(rc232Options, 10);

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

void menu_handler_sensors(void) {
  const char *sensorsOptions[] = {"[r]ead temperature (queue latest)"};
  menu_display(sensorsOptions, 1);

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

void menu_handler_time(void) {
  const char *timeOptions[] = {"rtc get [d]ate", "rtc get [t]ime"};
  menu_display(timeOptions, dimof(timeOptions));

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'd':
    ExtRTC_GetDate(&date_menu);
    // McuExtRTC_GetDate(&date_menu)
    printf("Date: %d.%d.%d\n", date_menu.Day, date_menu.Month, date_menu.Year);
    break;
  case 't':
    ExtRTC_GetTime(&time_menu);
    // McuExtRTC_GetTime(&time_menu)
    printf("Time: %d:%d:%d\n", time_menu.Hour, time_menu.Min, time_menu.Sec);
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}