#ifndef LOW_POWER_OPERATIONS_H_
#define LOW_POWER_OPERATIONS_H_

#include "pico/sleep.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

void low_power_init(void);
void low_power_sleep(void);

//void low_power_sleep_callback(void);
//void low_power_rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to);
//void low_power_rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to, bool *state);

//void low_power_recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig);
//void low_power_recover_from_sleep(void);
void clock_measure_freqs(void);

#endif /* LOW_POWER_OPERATIONS_H_ */