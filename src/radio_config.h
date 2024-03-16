#ifndef RADIO_H_
#define RADIO_H_

//#define STRING_LENGTH_UINT8 ((CHAR_BIT * sizeof(uint8_t) - 1) / 3 + 2)

//void radio_send_test_messages(void);
void radio_send(void);
void radio_reset(void);
void radio_init(void);
void radio_read_temperature(void);

#endif /* RADIO_H_ */