#ifndef RADIO_HANDLER_H_
#define RADIO_HANDLER_H_

void menu_display(const char *options[], int numOptions);
void menu_init(void);
char menu_get_user_input(void);
void menu_handler_main(void);
void menu_handler_radio(void);
void menu_handler_rc232(void);
void menu_handler_rc232_config(void);
void menu_handler_sensors(void);
void menu_handler_time(void);
void menu_handler_power(void);

#endif /* RADIO_HANDLER_H_ */