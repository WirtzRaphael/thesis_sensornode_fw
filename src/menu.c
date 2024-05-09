#include "menu.h"
#include "radio_config.h"
#include "stdio.h"
#include <stdint.h>

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
  const char *radioOptions[] = {"[b]uffer read out", "[r]eceive", "[s]end",
                                "[t]emperature",     "[v]oltage", "s[l]eep",
                                "[w]ake up"};
  menu_display(radioOptions, 7);

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'b':
    radio_uart_read_all();
    break;
  case 'l':
    radio_sleep();
    break;
  case 'r':
    // todo radio receive
    break;
  case 's':
    radio_send();
    break;
  case 't':
    radio_read_temperature();
    break;
  case 'v':
    radio_read_voltage();
    break;
  case 'w':
    radio_wakeup();
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
void menu_handler_radio_config(void) {
  const char *radioOptions[] = {"[c]hannel",
                                "[d]estination address",
                                "rss[i]",
                                "memory read [b]yte (NVM)",
                                "memory [w]rite (NVM)",
                                "[m]emory read config",
                                "[p]ower",
                                "[r]eset",
                                "e[x]it config state",
                                "[0] get config / check if config mode"};
  menu_display(radioOptions, 10);

  char userCmd = menu_get_user_input();
  uint8_t rssi;
  switch (userCmd) {
  case 'c':
    radio_config_rf_channel_number(5);
    break;
  case 'd':
    radio_config_destination_address(20);
    break;
  case 'i':
    rssi = radio_signal_strength_indicator();
    break;
  case 'b':
    radio_memory_read_one_byte(NVM_ADDR_RF_CHANNEL);
    radio_memory_read_one_byte(NVM_ADDR_RF_POWER);
    radio_memory_read_one_byte(NVM_ADDR_RF_DATA_RATE);
    break;
  case 'm':
    radio_memory_read_configuration();
    break;
  case 'p':
    radio_config_rf_power(1);
    break;
  case 'w':
    radio_memory_write_configuration();
    break;
  case 'r':
    radio_reset();
    break;
  case 'x':
    exit_config_state();
    break;
  case '0':
    radio_get_configuration_memory();
    break;
  default:
    printf("Invalid option\n");
    break;
  }
}