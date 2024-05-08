#include "app_platform.h"
#include <stdio.h>
#if PL_CONFIG_USE_PICO_W
  #include "pico/cyw43_arch.h" // before PicoWiFi.h
#endif
#if PL_CONFIG_USE_PICO_W
  #include "PicoWiFi.h"
#endif
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "application.h"
#include "pico_config.h"
#include "radio_config.h"
#include "menu.h"

#include "pico/stdlib.h"
#include "stdio.h"

#include "McuRTOS.h"
#include "application.h"
#if PL_CONFIG_USE_RTT
  #include "McuRTT.h"
#endif
#if PL_CONFIG_USE_LITTLE_FS
  #include "littleFS/McuLittleFS.h"
#endif
#if MCUW25Q128_CONFIG_ENABLED
  #include "McuW25Q128.h"
#endif

#include "McuLED.h"
#include "McuLog.h"
#include "McuUtility.h"
#include "hardware/gpio.h"

/*
 * Application
 */
#if !PL_CONFIG_USE_PICO_W
  #define LED_PIN (25) /* GPIO 25 */
#endif

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

  // Test McuW25Q128 communication
  uint8_t buffer_mcuw25[3];
  uint8_t errorCode = McuW25_ReadID(buffer_mcuw25, 3);
  if (errorCode != ERR_OK) {
    McuLog_trace("McuW25_ReadID error code: %d", errorCode);
  } else {
    McuLog_trace("McuW25_ReadID returned ID: %d %d %d", buffer_mcuw25[0],
                 buffer_mcuw25[1], buffer_mcuw25[2]);
  }
  McuLog_trace("McuW25_ReadID returned: %d", buffer_mcuw25[0]);

  for (;;) {
#if APP_HAS_ONBOARD_GREEN_LED
    McuLED_Toggle(led);
#elif PL_CONFIG_USE_PICO_W && !PL_CONFIG_USE_WIFI
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledIsOn);
    ledIsOn = !ledIsOn;
#endif
    vTaskDelay(pdMS_TO_TICKS(5 * 100));

    const char *mainMenuOptions[] = {"[r]adio"};
    menu_display(mainMenuOptions, 1);

    char userCmd = menu_get_user_input();
    switch (userCmd) {
    case 'r':
      menu_handler_radio();
      break;
    default:
      printf("Invalid option\n");
      break;
    }
  }
}

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

void APP_Run(void) {
  PL_Init();
#if PICO_CONFIG_USE_RADIO
  radio_init();
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
