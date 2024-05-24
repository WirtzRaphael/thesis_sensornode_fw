#include "application.h"
#include "app_platform.h"
#include "pico/time.h"
#include "pico_config.h"
#include "time_operations.h"
/*
#include <stdio.h>
*/
#if PL_CONFIG_USE_PICO_W
  #include "pico/cyw43_arch.h" // before PicoWiFi.h
#endif
#if PL_CONFIG_USE_PICO_W
  #include "PicoWiFi.h"
#endif
// tasks and dependencies
#if PICO_CONFIG_USE_POWER
  #include "power.h"
#endif
#if PICO_CONFIG_USE_RTC
  #include "extRTC.h"
#endif
#if PICO_CONFIG_USE_MENU
  #include "menu.h"
#endif
#if PICO_CONFIG_USE_RADIO
  #include "radio.h"
  #include "rc232.h"
#endif
#if PICO_CONFIG_USE_SENSORS
  #include "sensors.h"
#endif
// McuLib
#if PL_CONFIG_USE_BUTTONS
  #include "McuButton.h"
#endif
#include "McuArmTools.h"
#include "McuGenericI2C.h"
#include "McuLED.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "McuUtility.h"
#if PL_CONFIG_USE_RTT
  #include "McuRTT.h"
#endif
#if PL_CONFIG_USE_PCF85063A
  #include "McuPCF85063A.h"
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
  }
}
#endif

/**
 * \brief Application task.
 * \param pv rtos parameter
 */
static void AppTask(void *pv) {
/* -- TASK INIT -- */
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
  config.hw.pin = 17;
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
  /*
 uint8_t buffer_mcuw25[3];
 uint8_t errorCode = McuW25_ReadID(buffer_mcuw25, 3);
 if (errorCode != ERR_OK) {
   McuLog_trace("McuW25_ReadID error code: %d", errorCode);
 } else {
   McuLog_trace("McuW25_ReadID returned ID: %d %d %d", buffer_mcuw25[0],
                buffer_mcuw25[1], buffer_mcuw25[2]);
 }
 McuLog_trace("McuW25_ReadID returned: %d", buffer_mcuw25[0]);
 */

#if PL_CONFIG_USE_PCF85063A
  if (McuPCF85063A_WriteClockOutputFrequency(McuPCF85063A_COF_FREQ_OFF) !=
      ERR_OK) {
    McuLog_fatal("failed writing COF");
  }
#endif

#if PICO_CONFIG_USE_RTC
  TIMEREC time;
  TIMEREC time_alert = {0, 0, 5, 0};
  DATEREC date;

  if (McuTimeDate_Init() != ERR_OK) { /* do it inside task, as needs to talk
                                         over I2C to the external RTC */
    McuLog_fatal("failed initializing McuTimeDate");
  }
#endif
  static uint16_t xDelay_wakeup_ms = 4500;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xDelay_wakeup = pdMS_TO_TICKS(xDelay_wakeup_ms);

#if PL_CONFIG_USE_PCF85063A
  // todo : required ? periodic sync
  if (McuTimeDate_SyncWithExternalRTC() != ERR_OK) {
    McuLog_fatal("failed sync with external RTC");
  }
#endif

  gpio_init(PICO_PINS_LED_2);
  gpio_set_dir(PICO_PINS_LED_2, GPIO_OUT);
  gpio_put(PICO_PINS_LED_2, false);

  for (;;) {
    printf("AppTask Start\n");
#if APP_HAS_ONBOARD_GREEN_LED
    McuLED_Toggle(led);
#elif PL_CONFIG_USE_PICO_W && !PL_CONFIG_USE_WIFI
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledIsOn);
    ledIsOn = !ledIsOn;
#endif
    // - recheck alert settings (?)
    // wait until other tasks done
    // todo : use semaphore for task sync (?)
    vTaskDelay(pdMS_TO_TICKS(500));

    printf("[App] Power\n");
#if PICO_CONFIG_USE_POWER
    // indicate shutdown
    gpio_put(PICO_PINS_LED_2, true);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // todo : sleep components (radio)
    /* Components sleep
    */
    rc232_sleep();

    /* Wakeup alert
     */
    time_rtc_alarm_reset_flag(); // be sure that the flag is reset
    // fixme : avoid time shiff -> pass time and check or at beginning of task
    // todo : get time rtc at start of task, alert based on this -> time sync rtc
    // todo : check if alert in the future or already passed -> time sync rtc
    time_rtc_alarm_from_now(&time_alert);
    time_rtc_alarm_enable();

    gpio_put(PICO_PINS_LED_2, false);
  #if APP_SHUTDOWN_POWER
    /* Deinit
     */
    printf("[App] Deinit / Suspend\n");
    vTaskSuspendAll();
    sensors_deinit(); // -> I2C
    rc232_deinit(); // -> UART
    McuGenericI2C_Deinit(); // -> I2C

    /* SHUTDOWN : 3V3
     */
    printf("[App] Power off\n");
    power_3v3_1_enable(false);
    // fixme : delay until when tasks suspended -> sleep.h & rtc rp2040
    sleep_ms(xDelay_wakeup_ms); // tasks supsended
    McuLog_error("[App] No power off after %d seconds\n", xDelay_wakeup_ms);
    McuArmTools_SoftwareReset(); /* restart */
    // fallback shutdown
    // - avoid deadlock and to re-initialize system
  #else
    // fixme : no sync with rtc time (!), periodic call by rtos
    printf("[App] Delay wakeup\n");
    vTaskDelayUntil(&xLastWakeTime, xDelay_wakeup);
    
    /* Wakeup 
    */
    rc232_wakeup();
  #endif
    /* NO CODE HERE*/
#endif

    // reset alarm flag for power cycle and restart program
    // avoid deadlock
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
#if PICO_CONFIG_USE_POWER
  power_init();
#endif
#if PL_CONFIG_USE_BUTTONS
  McuBtn_Init();
#endif
#if PICO_CONFIG_USE_SENSORS
  sensors_init(); // --> Sensor Task
#endif
#if PICO_CONFIG_USE_RADIO
  rc232_init();
  radio_init(); // --> Radio Task
#endif
#if PICO_CONFIG_USE_RTC
  // todo : move
  gpio_init(PICO_PINS_I2C0_ENABLE);
  gpio_set_dir(PICO_PINS_I2C0_ENABLE, GPIO_OUT);
  gpio_put(PICO_PINS_I2C0_ENABLE, 1);
  ExtRTC_Init(); // --> Timer Service Task, already in app platform)
#endif
#if PICO_CONFIG_USE_MENU
  menu_init(); // --> Menu Task
#endif

// fixme : if semaphore null also check/deactivate later or stop here
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
