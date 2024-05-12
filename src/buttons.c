/*
 * Copyright (c) 2022, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_platform.h"
#if PL_CONFIG_USE_BUTTONS
#include "buttons.h"
#include "buttons_config.h"
#include <assert.h>
#include "McuButton.h"
#include "McuRTOS.h"
#include "debounce.h"
#if McuLib_CONFIG_CPU_IS_KINETIS
  #include "fsl_port.h"
#elif McuLib_CONFIG_CPU_IS_LPC
  #include "fsl_pint.h"
  #include "fsl_syscon.h"
#endif
#if configUSE_SEGGER_SYSTEM_VIEWER_HOOKS
  #include "McuSystemView.h"
#endif

typedef struct BTN_Desc_t {
  McuBtn_Handle_t handle; /* handle of button pin */
} BTN_Desc_t;

static BTN_Desc_t BTN_Infos[BTN_NOF_BUTTONS];

bool BTN_IsPressed(BTN_Buttons_e btn) {
  assert(btn<BTN_NOF_BUTTONS);
  return McuBtn_IsOn(BTN_Infos[btn].handle);
}

uint32_t BTN_GetButtons(void) {
  uint32_t val = 0;

  if (BTN_IsPressed(BTN_A)) {
    val |= BTN_BIT_A;
  }
  if (BTN_IsPressed(BTN_B)) {
    val |= BTN_BIT_B;
  }
  if (BTN_IsPressed(BTN_C)) {
    val |= BTN_BIT_C;
  }
  return val;
}

static void SysViewLogStart(void) {
#if configUSE_SEGGER_SYSTEM_VIEWER_HOOKS
  #define MCU_SYSTEM_VIEW_USER_ID_BUTTON_INTERRUPT (0)
  McuSystemView_RecordEnterISR();
  McuSystemView_Print((const char*)"Pressed button\r\n");
  McuSystemView_OnUserStart(MCU_SYSTEM_VIEW_USER_ID_BUTTON_INTERRUPT);
#endif
}

static void SysViewLogEnd(void) {
#if configUSE_SEGGER_SYSTEM_VIEWER_HOOKS
  McuSystemView_OnUserStop(MCU_SYSTEM_VIEW_USER_ID_BUTTON_INTERRUPT);
  McuSystemView_RecordExitISR();
#endif
}

static void gpio_IsrCallback(uint gpio, uint32_t events) {
  uint32_t button = 0; /* init */
  BaseType_t xHigherPriorityTaskWoken = false;

  switch(gpio) {
    case BUTTONS_A_PIN:
      button = BTN_BIT_A;
      break;
    case BUTTONS_B_PIN:
      button = BTN_BIT_B;
      break;
    case BUTTONS_C_PIN:
      button = BTN_BIT_C;
      break;
    default:
      button = 0;
      break;
  }
  if (button!=0) {
    Debounce_StartDebounceFromISR(button, &xHigherPriorityTaskWoken);
  }
}

void BTN_Deinit(void) {
  for(int i=0; i<BTN_NOF_BUTTONS; i++) {
    BTN_Infos[i].handle = McuBtn_DeinitButton(BTN_Infos[i].handle);
  }
}

void BTN_Init(void) {
  McuBtn_Config_t btnConfig;

  BUTTONS_ENABLE_CLOCK();
  McuBtn_GetDefaultConfig(&btnConfig);
  btnConfig.isLowActive = true;

  btnConfig.hw.pin = BUTTONS_A_PIN;
  btnConfig.hw.pull = McuGPIO_PULL_UP;
  BTN_Infos[BTN_A].handle = McuBtn_InitButton(&btnConfig);

  btnConfig.hw.pin = BUTTONS_B_PIN;
  btnConfig.hw.pull = McuGPIO_PULL_UP;
  BTN_Infos[BTN_B].handle = McuBtn_InitButton(&btnConfig);

  btnConfig.hw.pin = BUTTONS_C_PIN;
  btnConfig.hw.pull = McuGPIO_PULL_UP;
  BTN_Infos[BTN_C].handle = McuBtn_InitButton(&btnConfig);

  gpio_set_irq_enabled_with_callback(BUTTONS_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_IsrCallback);
  gpio_set_irq_enabled_with_callback(BUTTONS_B_PIN,     GPIO_IRQ_EDGE_FALL, true, &gpio_IsrCallback);
  gpio_set_irq_enabled_with_callback(BUTTONS_C_PIN,   GPIO_IRQ_EDGE_FALL, true, &gpio_IsrCallback);
}

#endif /* #if PL_CONFIG_USE_BUTTONS */
