#include "app_platform.h"
#include <stdio.h>
#if PL_CONFIG_USE_PICO_W
  #include "pico/cyw43_arch.h" // before PicoWiFi.h
#endif
#if PL_CONFIG_USE_PICO_W
  #include "PicoWiFi.h"
#endif

#include "pico_config.h"
// tasks and dependencies
#include "application.h"
#include "extRTC.h"
#include "menu.h"
#include "radio.h"
#include "rc232.h"
#include "sensors.h"
// McuLib
#include "McuButton.h"
#include "McuRTOS.h"
// #include "McuLED.h"
#include "McuLog.h"
#include "McuUtility.h"
#if PL_CONFIG_USE_RTT
  #include "McuRTT.h"
#endif
#if PL_CONFIG_USE_LITTLE_FS
  #include "littleFS/McuLittleFS.h"
#endif
#if MCUW25Q128_CONFIG_ENABLED
  #include "McuW25Q128.h"
#endif

#if PL_CONFIG_USE_BUTTONS
#include "semphr.h"
// todo review : extern definition in header and here
SemaphoreHandle_t xButtonASemaphore;
SemaphoreHandle_t xButtonBSemaphore;
SemaphoreHandle_t xButtonCSemaphore;
/**
 * \brief Called by the button driver if a button event is detected.
 * \param button Button for which the event is detected.
 * \param kind Event kind.
 */
void APP_OnButtonEvent(BTN_Buttons_e button, McuDbnc_EventKinds kind) {
  unsigned char buf[32];
  buf[0] = '\0';
  switch (button) {
  case BTN_A:
    McuUtility_strcat(buf, sizeof(buf), "A");
    break;
  case BTN_B:
    McuUtility_strcat(buf, sizeof(buf), "B");
    break;
  case BTN_C:
    McuUtility_strcat(buf, sizeof(buf), "C");
    break;
  default:
    McuUtility_strcat(buf, sizeof(buf), "???");
    break;
  }
  switch (kind) {
  case MCUDBNC_EVENT_PRESSED:
    McuUtility_strcat(buf, sizeof(buf), " pressed");
    break;
  case MCUDBNC_EVENT_PRESSED_REPEAT:
    McuUtility_strcat(buf, sizeof(buf), " pressed-repeat");
    break;
  case MCUDBNC_EVENT_LONG_PRESSED:
    McuUtility_strcat(buf, sizeof(buf), " pressed-long");
    break;
  case MCUDBNC_EVENT_LONG_PRESSED_REPEAT:
    McuUtility_strcat(buf, sizeof(buf), " pressed-long-repeat");
    break;
  case MCUDBNC_EVENT_RELEASED:
    McuUtility_strcat(buf, sizeof(buf), " released");
    break;
  case MCUDBNC_EVENT_LONG_RELEASED:
    McuUtility_strcat(buf, sizeof(buf), " long released");
    break;
  default:
    McuUtility_strcat(buf, sizeof(buf), "???");
    break;
  }
  McuUtility_strcat(buf, sizeof(buf), "\n");
  #if 0 && PL_CONFIG_USE_RTT /* debugging only */
  McuRTT_printf(0, buf);
  #endif

  // todo : refactor, remove printf
  if (button == BTN_A && kind == MCUDBNC_EVENT_RELEASED) {
    printf("[app] Semaphore give A\n");
    McuLog_info("[app] Semaphore give Button A");
    xSemaphoreGive(xButtonASemaphore);
  } else if (button == BTN_B && kind == MCUDBNC_EVENT_PRESSED) {
    printf("[app] Semaphore give B\n");
    McuLog_info("[app] Semaphore give Button B");
    xSemaphoreGive(xButtonBSemaphore);
  } else if (button == BTN_C && kind == MCUDBNC_EVENT_PRESSED) {
    printf("[app] Semaphore give C\n");
    McuLog_info("[app] Semaphore give Button C");
    xSemaphoreGive(xButtonCSemaphore);
  }
}
#endif

/**
 * \brief Application task.
 * \param pv rtos parameter
 */
static void AppTask(void *pv) {
/* -- TASK INIT -- */
#define APP_HAS_ONBOARD_GREEN_LED (!PL_CONFIG_USE_PICO_W)
#if !PL_CONFIG_USE_WIFI && PL_CONFIG_USE_PICO_W
  if (cyw43_arch_init() ==
      0) { /* need to init for accessing LEDs and other pins */
    PicoWiFi_SetArchIsInitialized(true);
  } else {
    McuLog_fatal("failed initializing CYW43");
    for (;;) {
    }
  }
#endif

#if APP_HAS_ONBOARD_GREEN_LED
  /* only for pico boards which have an on-board green LED */
  McuLED_Config_t config;
  McuLED_Handle_t led;

  McuLED_GetDefaultConfig(&config);
  config.hw.pin = LED_PIN;
  config.isLowActive = false;
  led = McuLED_InitLed(&config);
  if (led == NULL) {
    for (;;) {
    }
  }
#elif PL_CONFIG_USE_PICO_W && !PL_CONFIG_USE_WIFI
  bool ledIsOn = false;
#endif

#if PL_CONFIG_USE_LITTLE_FS
  McuLog_info("Mounting litteFS volume.");
  if (McuLFS_Mount(McuShell_GetStdio()) == ERR_FAILED) {
    McuLog_info("Mounting failed please format device first");
  }
#endif
  /* Test McuW25Q128 communication
   */
  uint8_t buffer_mcuw25[3];
  uint8_t errorCode = McuW25_ReadID(buffer_mcuw25, 3);
  if (errorCode != ERR_OK) {
    McuLog_trace("McuW25_ReadID error code: %d", errorCode);
  } else {
    McuLog_trace("McuW25_ReadID returned ID: %d %d %d", buffer_mcuw25[0],
                 buffer_mcuw25[1], buffer_mcuw25[2]);
  }
  McuLog_trace("McuW25_ReadID returned: %d", buffer_mcuw25[0]);

  TIMEREC time;
  DATEREC date;

  for (;;) {
#if APP_HAS_ONBOARD_GREEN_LED
    McuLED_Toggle(led);
#elif PL_CONFIG_USE_PICO_W && !PL_CONFIG_USE_WIFI
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledIsOn);
    ledIsOn = !ledIsOn;
#endif
    vTaskDelay(pdMS_TO_TICKS(5 * 100));
  }
} /* AppTask */

/**
 * \brief Shell command to print status information.
 */
static uint8_t PrintStatus(McuShell_ConstStdIOType *io) {
  unsigned char buf[48];

  McuShell_SendStatusStr((unsigned char *)"app",
                         (const unsigned char *)"Status of application\r\n",
                         io->stdOut);
  McuUtility_Num32uToStr(buf, sizeof(buf), PL_CONFIG_HW_ACTIVE_HW_VERSION / 10);
  McuUtility_chcat(buf, sizeof(buf), '.');
  McuUtility_strcatNum8u(buf, sizeof(buf), PL_CONFIG_HW_ACTIVE_HW_VERSION % 10);
  McuUtility_strcat(buf, sizeof(buf), (unsigned char *)"\r\n");
  McuShell_SendStatusStr((uint8_t *)"  HW", (unsigned char *)buf, io->stdOut);

  McuUtility_strcpy(buf, sizeof(buf), (unsigned char *)APP_VERSION_STR);
  McuUtility_strcat(buf, sizeof(buf), (unsigned char *)"\r\n");
  McuShell_SendStatusStr((uint8_t *)"  version", buf, io->stdOut);

  return ERR_OK;
}

/**
 * \brief Shell command to parse application specific commands.
 */
uint8_t App_ParseCommand(const unsigned char *cmd, bool *handled,
                         const McuShell_StdIOType *io) {
  uint32_t value;
  const unsigned char *p;

  if (McuUtility_strcmp((char *)cmd, McuShell_CMD_HELP) == 0 ||
      McuUtility_strcmp((char *)cmd, "app help") == 0) {
    McuShell_SendHelpStr(
        (unsigned char *)"app",
        (const unsigned char *)"Group of application commands\r\n", io->stdOut);
    McuShell_SendHelpStr(
        (unsigned char *)"  help|status",
        (const unsigned char *)"Print help or status information\r\n",
        io->stdOut);
    *handled = true;
    return ERR_OK;
  } else if ((McuUtility_strcmp((char *)cmd, McuShell_CMD_STATUS) == 0) ||
             (McuUtility_strcmp((char *)cmd, "app status") == 0)) {
    *handled = true;
    return PrintStatus(io);
  }
  return ERR_OK;
}

/**
 * \brief Appplication main function.
 */
void APP_Run(void) {
  PL_Init();
  McuBtn_Init();
  sensors_init();
#if PICO_CONFIG_USE_RADIO
  rc232_init();
  radio_init();
#endif
  ExtRTC_Init(); // --> Timer Service Task

#if PL_CONFIG_USE_BUTTONS
xButtonASemaphore = xSemaphoreCreateBinary();
if (xButtonASemaphore == NULL) {
  McuLog_fatal("failed creating semaphore");
}
xButtonBSemaphore = xSemaphoreCreateBinary();
if (xButtonBSemaphore == NULL) {
  McuLog_fatal("failed creating semaphore");
}
xButtonCSemaphore = xSemaphoreCreateBinary();
if (xButtonCSemaphore == NULL) {
  McuLog_fatal("failed creating semaphore");
}
#endif

  McuLog_info("Create task 'App' ... ");

  if (xTaskCreate(AppTask, /* pointer to the task */
                  "App",   /* task name for kernel awareness debugging */
                  1500 / sizeof(StackType_t), /* task stack size */
                  (void *)NULL,         /* optional task startup argument */
                  tskIDLE_PRIORITY + 2, /* initial priority */
                  (TaskHandle_t *)NULL  /* optional task handle to create */
                  ) != pdPASS) {
    McuLog_fatal("failed creating task");
    for (;;) {
    } /* error! probably out of memory */
  }
  vTaskStartScheduler();
  for (;;) {
    /* shall not get here */
  }
}
