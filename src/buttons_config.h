/*
 * Copyright (c) 2022, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BUTTONS_CONFIG_H_
#define BUTTONS_CONFIG_H_

#include "app_platform.h"
#include "pico_config.h"

#if PL_CONFIG_USE_BUTTONS

  /* GPIO pins for buttons */
  #define BUTTONS_A_PIN (PICO_PINS_BUTTON_A)
  #define BUTTONS_B_PIN (PICO_PINS_BUTTON_B)

  #define BUTTONS_ENABLE_CLOCK() /* enable clocking */

typedef enum {
  BTN_A,
  BTN_B,
  BTN_NOF_BUTTONS /* sentinel, must be last in list! */
} BTN_Buttons_e;

  /* bits of the buttons */
  #define BTN_BIT_A (1 << 0)
  #define BTN_BIT_B (1 << 1)

#endif /* PL_CONFIG_USE_BUTTONS */

#endif /* BUTTONS_CONFIG_H_ */
