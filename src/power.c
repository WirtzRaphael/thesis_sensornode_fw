/**
 * @file radio.c
 * @author raphael wirtz
 * @brief Power management
 * @date 2024-05-22
 *
 * @copyright Copyright (c) 2024
 */

#include "platform_config.h"

#if PLATFORM_CONFIG_USE_POWER
  #include "McuLog.h"
  #include "pico/sleep.h"
  #include "pico/stdlib.h"
  #include "platform_config.h"
  #include "power.h"
  #include "stdio.h"
  #include "time_operations.h"
  #include <errno.h>

  #if PLATFORM_CONFIG_USE_RTC
  // #include "extRTC.h"
    #include "McuPCF85063A.h"
  #endif

static bool periodic_shutdown = APP_POWER_AUTO_SHUTDOWN;
//static bool periodic_shutdown = true;

void power_init(void) {
  /* Pin : 3V3 Power enable
   * note : HOLDS THE POWER SUPPLY
   * fix : uses output to led also for power enable
   */
  gpio_init(PICO_PINS_LED_1);
  gpio_set_dir(PICO_PINS_LED_1, GPIO_OUT);
  // 3V3 : enable
  gpio_put(PICO_PINS_LED_1, true);
  // power_3v3_1_enable(true);

  /* Pin : 3V3 Power mode
   * fix : use RS232 Pin
   */
  gpio_init(PICO_PINS_RS232_FORCEOFF_N);
  gpio_set_dir(PICO_PINS_RS232_FORCEOFF_N, GPIO_OUT);
  // power_mode(POWER_MODE_LIGHT);
}

void power_init_at_runtime(void) {
  /* RTC
   */
  time_rtc_alarm_reset_flag(); // be sure that the flag is reset

  // todo : move ?
  if (McuPCF85063A_WriteClockOutputFrequency(McuPCF85063A_COF_FREQ_OFF) !=
      ERR_OK) {
    McuLog_fatal("failed writing COF");
  }
}

void power_set_periodic_shutdown(bool shutdown) {
  periodic_shutdown = shutdown;
}

bool power_get_periodic_shutdown(void) { return periodic_shutdown; }

void power_toggle_periodic_shutdown() {
  periodic_shutdown = !periodic_shutdown;
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
error_t power_3V3_mode(power_3V3_mode_t mode) {
  switch (mode) {
  case POWER_MODE_LIGHT:
    gpio_put(PICO_PINS_RS232_FORCEOFF_N, false);
    return ERR_OK;
    break;
  case POWER_MODE_HEAVY:
    gpio_put(PICO_PINS_RS232_FORCEOFF_N, true);
    return ERR_OK;
    break;
  default:
    McuLog_error("Invalid power mode.");
  }
  return ERR_FAILED;
}

void power_sleep(void) {
  // todo
}

#endif /* PLATFORM_CONFIG_USE_POWER */