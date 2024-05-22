/**
 * @file radio.c
 * @author raphael wirtz
 * @brief Power management
 * @date 2024-05-22
 *
 * @copyright Copyright (c) 2024
 */

#include "McuLog.h"
#include "pico/stdlib.h"
#include "pico_config.h"
#include "stdio.h"
#include <errno.h>

#if PICO_CONFIG_USE_POWER

// todo : move central
void power_init(void) {
  /* Pin : 3V3 Power enable
   * note : HOLDS THE POWER SUPPLY
   * fix : uses output to led also for power enable
   */
  gpio_init(PICO_PINS_LED_1);
  gpio_set_dir(PICO_PINS_LED_1, GPIO_OUT);
  // 3V3 : enable
  gpio_put(PICO_PINS_LED_1, true);

  /* Pin : 3V3 Power mode
   * fix : use RS232 Pin
   */
  gpio_init(PICO_PINS_RS232_FORCEOFF_N);
  gpio_set_dir(PICO_PINS_RS232_FORCEOFF_N, GPIO_OUT);
  // power mode : light
  gpio_put(PICO_PINS_RS232_FORCEOFF_N, false); // power mode light
}

// todo : function 3V3 power mode light/heavy
// fix : use led

// todo : 3V3-1 deactivate power
// fix : use rs232 enable pin

// todo (cancelled) : 3V3-2 activate

#endif /* PICO_CONFIG_USE_POWER */