#ifndef RADIO_H_
#define RADIO_H_
// #define STRING_LENGTH_UINT8 ((CHAR_BIT * sizeof(uint8_t) - 1) / 3 + 2)

// void radio_send_test_messages(void);
/*! Sends a message via radio 
*/
void radio_send(void);
void radio_reset(void);
void radio_init(void);
void radio_read_temperature(void);
void radio_uart_read_all(void);
void radio_memory_read_one_byte(uint8_t address);
void radio_memory_configuration(void);

#endif /* RADIO_H_ */