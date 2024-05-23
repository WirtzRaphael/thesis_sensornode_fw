/**
 * @file radio.c
 * @author raphael wirtz
 * @brief Power management
 * @date 2024-05-22
 *
 * @copyright Copyright (c) 2024
 */

#include "power.h"
#include "McuLog.h"
#include "pico/stdlib.h"
#include "pico_config.h"
#include "stdio.h"
#include <errno.h>

#if PICO_CONFIG_USE_POWER

void power_init(void) {
  /* Pin : 3V3 Power enable
   * note : HOLDS THE POWER SUPPLY
   * fix : uses output to led also for power enable
   */
  gpio_init(PICO_PINS_LED_1);
  gpio_set_dir(PICO_PINS_LED_1, GPIO_OUT);
  // 3V3 : enable
  gpio_put(PICO_PINS_LED_1, true);
  //power_3v3_1_enable(true);

  /* Pin : 3V3 Power mode
   * fix : use RS232 Pin
   */
  gpio_init(PICO_PINS_RS232_FORCEOFF_N);
  gpio_set_dir(PICO_PINS_RS232_FORCEOFF_N, GPIO_OUT);
  //power_mode(POWER_MODE_LIGHT);
}

/**
 * @brief Enable or disable 3V3-1 power supply
 * 
 * @param enable true to enable, false to disable
 *
 * fix : use led pin
 */
void power_3v3_1_enable(bool enable) {
  if (enable) {
    gpio_put(PICO_PINS_LED_1, true);
  } else {
    gpio_put(PICO_PINS_LED_1, false);
  }
}

/**
 * @brief Enable or disable 3V3-2 power supply
 * 
 * @param enable true to enable, false to disable
 */
void power_3v3_2_enable(bool enable) {
  // Component not present
  McuLog_error("Not implemented.");
}

/**
 * @brief Set power mode
 * 
 * @param mode power mode
 * @return error_t 
 *
 * fix : use rs232 enable pin
 */
error_t power_mode(power_mode_t mode) {
  switch (mode) {
  case POWER_MODE_LIGHT:
    gpio_put(PICO_PINS_RS232_FORCEOFF_N, false);
    break;
  case POWER_MODE_HEAVY:
    gpio_put(PICO_PINS_RS232_FORCEOFF_N, true);
    break;
  default:
    McuLog_error("Invalid power mode.");
  }
}

// todo : rtc wakeup

#endif /* PICO_CONFIG_USE_POWER */