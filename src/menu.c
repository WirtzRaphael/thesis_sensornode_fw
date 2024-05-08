#include "menu.h"
#include "radio_config.h"
#include "stdio.h"

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
 * @brief
 *
 */
void menu_handler_radio(void) {
  const char *radioOptions[] = {"[b]uffer read out",
                                "[m]emory read",
                                "memory [w]rite",
                                "[r]eset",
                                "[s]end",
                                "[t]emperature",
                                "[d]estination address"};
  menu_display(radioOptions, 7);

  char userCmd = menu_get_user_input();
  switch (userCmd) {
  case 'b':
    radio_uart_read_all();
    break;
  case 'd':
    radio_config_destination_address(123);
    break;
  case 'm':
    radio_memory_read_one_byte(0x00);
    radio_memory_read_one_byte(0x01);
    radio_memory_read_one_byte(0x02);
    break;
  case 'r':
    radio_reset();
    break;
  case 's':
    radio_send();
    break;
  // Add other cases as needed
  default:
    printf("Invalid option\n");
    break;
  }
}