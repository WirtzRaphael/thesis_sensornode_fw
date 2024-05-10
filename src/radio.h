#ifndef RADIO_H_
#define RADIO_H_

#include <errno.h>

void radio_authentication(void);
static void radio_send_authentication_request(void);
static error_t radio_wait_for_authentication_response(void);
void radio_init(void);
void radio_send_temperature(void);
void radio_send_test(void);

#endif /* RADIO_H_ */