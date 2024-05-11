#include "menu.h"
#include "rc232.h"
#include "radio.h"
#include "stdio.h"
#include <stdint.h>

#include "pico/stdlib.h"

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
 * @brief menu for radio features
 *
 */
void menu_handler_radio(void) {
  const char *radioOptions[] = {"[a]uthenticate", "[s]end",
                                "send [t]est message"};
  menu_display(radioOptions, 3);

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'a':
    radio_authentication();
    break;
  case 's':
    radio_send_temperature();
    break;
  case 't':
    radio_send_test();
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
                                "broad[c]ast",    "[r]eceive",
                                "[s]end",         "[t]emperature",
                                "[v]oltage",      "s[l]eep",
                                "[w]ake up"};
  menu_display(rc232Options, 8);

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'b':
    rc232_rx_read_buffer_full();
    break;
  case 'c':
    rc232_config_destination_address(RC232_BROADCAST_ADDRESS);
    radio_send_test();
    rc232_config_destination_address(radio_get_rf_destination_address()); // set back to default
    break;
  case 'l':
    rc232_sleep();
    break;
  case 'r':
    rc232_rx_read_buffer_full(); // same as buffer read out
    break;
  case 's':
    rc232_tx_string("RC232 Send String");
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