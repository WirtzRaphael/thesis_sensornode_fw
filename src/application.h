/*
 * Copyright (c) 2022, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "McuShell.h"
#include "app_platform.h"

#if PL_CONFIG_USE_BUTTONS
  #include "McuDebounce.h"
  #include "buttons.h"
  #include "semphr.h"
extern SemaphoreHandle_t xButtonASemaphore;
extern SemaphoreHandle_t xButtonAHoldSemaphore;
extern SemaphoreHandle_t xButtonBSemaphore;

void APP_OnButtonEvent(BTN_Buttons_e button, McuDbnc_EventKinds kind);
#endif

#define APP_VERSION_STR "v1.0b"

uint8_t App_GetSensorValues(float *temperature, float *humidity);

uint8_t App_ParseCommand(const unsigned char *cmd, bool *handled,
                         const McuShell_StdIOType *io);

void APP_Run(void);

#endif /* __APPLICATION_H__ */
