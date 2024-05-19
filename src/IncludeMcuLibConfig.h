/*
 * Copyright (c) 2023, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* header file is included with -include compiler option */

#ifndef MCULIB_CONFIG_CONFIG_H_
#define MCULIB_CONFIG_CONFIG_H_

#define PL_CONFIG_HW_AEMBS_BOARD    (0)
#define PL_CONFIG_HW_VERSION_1_0    (10)
#define PL_CONFIG_HW_VERSION_2_0    (20)
#define LIB_CONFIG_HW_VERSION  (PL_CONFIG_HW_VERSION_2_0)

/* ---------------------------------------------------------------------------------------*/
/* SDK */
#define McuLib_CONFIG_CPU_IS_KINETIS                (0)
#define McuLib_CONFIG_CORTEX_M                      (0) /* RP2040 is a Cortex-M0+ */
#define McuLib_CONFIG_CPU_IS_RPxxxx                 (1)
#define McuLib_CONFIG_CPU_VARIANT                   McuLib_CONFIG_CPU_VARIANT_RP2040
#define McuLib_CONFIG_SDK_VERSION_USED              McuLib_CONFIG_SDK_RPI_PICO
/* ---------------------------------------------------------------------- */
/* FreeRTOS */
#define McuLib_CONFIG_SDK_USE_FREERTOS              (1)
#define configMINIMAL_STACK_SIZE                    (500/sizeof(StackType_t))
#define configTIMER_TASK_STACK_DEPTH                (800/sizeof(StackType_t)) /* stack size for Timer Service task */
#define configTOTAL_HEAP_SIZE                       (64*1024)
#define configUSE_SEGGER_SYSTEM_VIEWER_HOOKS        (1)
/* -------------------------------------------------*/
/* I2C */
#define CONFIG_USE_HW_I2C                           (1) /* if using HW I2C, otherwise use software bit banging */
#define MCUI2CLIB_CONFIG_I2C_DEVICE                 i2c0
#define MCUI2CLIB_CONFIG_SDA_GPIO_PIN               16u
#define MCUI2CLIB_CONFIG_SCL_GPIO_PIN               17u
#define MCUI2CLIB_CONFIG_ADD_DELAY_US               (0)
#define MCUI2CLIB_CONFIG_TIMEOUT_BYTE_US            (1000)
/* -------------------------------------------------*/
/* McuGenericI2C */
#define McuGenericI2C_CONFIG_USE_ON_ERROR_EVENT       (0)
#define McuGenericI2C_CONFIG_USE_ON_RELEASE_BUS_EVENT (0)
#define McuGenericI2C_CONFIG_USE_ON_REQUEST_BUS_EVENT (0)
#define McuGenericI2C_CONFIG_USE_MUTEX                (1 && McuLib_CONFIG_SDK_USE_FREERTOS)

#if CONFIG_USE_HW_I2C /* implementation in McuI2cLib.c */
  #define McuLib_CONFIG_MCUI2CLIB_ENABLED                       (1)
  #define McuGenericI2C_CONFIG_INTERFACE_HEADER_FILE            "McuI2cLib.h"
  #define McuGenericI2C_CONFIG_RECV_BLOCK                       McuI2cLib_RecvBlock
  #define McuGenericI2C_CONFIG_SEND_BLOCK                       McuI2cLib_SendBlock
  #if McuGenericI2C_CONFIG_SUPPORT_STOP_NO_START
  #define McuGenericI2C_CONFIG_SEND_BLOCK_CONTINUE              McuI2cLib_SendBlockContinue
  #endif
  #define McuGenericI2C_CONFIG_SEND_STOP                        McuI2cLib_SendStop
  #define McuGenericI2C_CONFIG_SELECT_SLAVE                     McuI2cLib_SelectSlave
  #define McuGenericI2C_CONFIG_RECV_BLOCK_CUSTOM_AVAILABLE      (0)
  #define McuGenericI2C_CONFIG_RECV_BLOCK_CUSTOM                McuI2cLib_RecvBlockCustom

  #define MCUI2CLIB_CONFIG_ADD_DELAY                            (0)
#else
  /* settings for McuGenericSWI2C */
  #define SDA1_CONFIG_PIN_NUMBER  (16)
  #define SCL1_CONFIG_PIN_NUMBER  (17)

  #define McuGenericSWI2C_CONFIG_DO_YIELD (0 && McuLib_CONFIG_SDK_USE_FREERTOS) /* because of Yield in GenericSWI2C */
  #define McuGenericSWI2C_CONFIG_DELAY_NS (0)
#endif
/* -------------------------------------------------*/
/* Time/Date */
#define McuTimeDate_CONFIG_USE_SOFTWARE_RTC                        (1) /* enable software RTC */
#define McuTimeDate_CONFIG_USE_EXTERNAL_HW_RTC                     (1)
#define McuTimeDate_CONFIG_USE_INTERNAL_HW_RTC                     (0) /* no internal RTC */

#if McuTimeDate_CONFIG_USE_EXTERNAL_HW_RTC
  #define McuTimeDate_CONFIG_INIT_SOFTWARE_RTC_METHOD                (McuTimeDate_INIT_SOFTWARE_RTC_FROM_DEFAULTS) /* initialize first from software defaults, will update later from HW RTC */
  #define McuTimeDate_CONFIG_EXT_RTC_HEADER_FILE_NAME                "../../src/extRTC.h"
  #define McuTimeDate_CONFIG_EXT_RTC_GET_TIME_FCT                    ExtRTC_GetTime
  #define McuTimeDate_CONFIG_EXT_RTC_SET_TIME_FCT                    ExtRTC_SetTimeInfo
  #define McuTimeDate_CONFIG_EXT_RTC_GET_DATE_FCT                    ExtRTC_GetDate
  #define McuTimeDate_CONFIG_EXT_RTC_SET_DATE_FCT                    ExtRTC_SetDateInfo
#else
  #define McuTimeDate_CONFIG_INIT_SOFTWARE_RTC_METHOD                (McuTimeDate_INIT_SOFTWARE_RTC_FROM_DEFAULTS)
#endif
#define McuTimeDate_CONFIG_USE_GET_TIME_DATE_METHOD                (McuTimeDate_GET_TIME_DATE_METHOD_SOFTWARE_RTC)
//#define McuTimeDate_CONFIG_USE_GET_TIME_DATE_METHOD                (McuTimeDate_GET_TIME_DATE_METHOD_EXTERNAL_RTC) // problem, i2c sensor stops. cyw43 load
#define McuTimeDate_CONFIG_SET_TIME_DATE_METHOD_USES_SOFTWARE_RTC  (1) /* if using software RTC */
#define McuTimeDate_CONFIG_SET_TIME_DATE_METHOD_USES_EXTERNAL_RTC  (McuTimeDate_CONFIG_USE_EXTERNAL_HW_RTC) /* if using external I2C RTC */
#define McuTimeDate_CONFIG_SET_TIME_DATE_METHOD_USES_INTERNAL_RTC  (0) /* if using internal HW RTC */

/* start values */
#define McuTimeDate_CONFIG_DEFAULT_INITIAL_TIME_HOUR  0
#define McuTimeDate_CONFIG_DEFAULT_INITIAL_TIME_MIN   0
#define McuTimeDate_CONFIG_DEFAULT_INITIAL_TIME_SEC   0
#define McuTimeDate_CONFIG_DEFAULT_INITIAL_DATE_YEAR  2020
#define McuTimeDate_CONFIG_DEFAULT_INITIAL_DATE_MONTH 1
#define McuTimeDate_CONFIG_DEFAULT_INITIAL_DATE_DAY   1
/* ---------------------------------------------------------------------------------------*/
/* McuSSD1306 */
#define McuSSD1306_CONFIG_SSD1306_DRIVER_TYPE           (1106)
#define McuSSD1306_CONFIG_DYNAMIC_DISPLAY_ORIENTATION   (0)
#define McuSSD1306_CONFIG_FIXED_DISPLAY_ORIENTATION     McuSSD1306_CONFIG_ORIENTATION_LANDSCAPE
/* -------------------------------------------------*/
/* RTT */
#define McuRTT_CONFIG_RTT_BUFFER_SIZE_DOWN            (128)
#define McuRTT_CONFIG_BLOCKING_SEND                   (1) /* 0: do not block if buffer full */
#define McuRTT_CONFIG_BLOCKING_SEND_TIMEOUT_MS        (5)
#define McuRTT_CONFIG_BLOCKING_SEND_WAIT_MS           (1)
#define McuRTT_CONFIG_RTT_BUFFER_SIZE_UP              (1024)
/* ---------------------------------------------------------------------------------------*/
/* McuFlash */
#define McuFlash_CONFIG_IS_ENABLED                    (1) /* enable McuFlash module */
#define McuFlash_CONFIG_NOF_BLOCKS                    (32) /* number of flash blocks */
#define McuFlash_CONFIG_MEM_START                     (((0+244*1024)-((McuFlash_CONFIG_NOF_BLOCKS)*(McuFlash_CONFIG_FLASH_BLOCK_SIZE))))
/* ---------------------------------------------------------------------------------------*/
/* McuSPI */
#define MCUSPI_CONFIG_HW_TEMPLATE   MCUSPI_CONFIG_HW_TEMPLATE_RP2040_SPI1
// overwrite template values
#define MCUSPI_CONFIG_TRANSFER_BAUDRATE (24*1000*1000)
#define MCUSPI_CONFIG_HW_SCLK_PIN (14)  /* SPI1_SCK */
#define MCUSPI_CONFIG_HW_MOSI_PIN (15)  /* SPI1_TX  */
#define MCUSPI_CONFIG_HW_MISO_PIN (12)  /* SPI1_RX  */
#define MCUSPI_CONFIG_HW_CS_PIN   (13)  /* SPI1_CSn */
/* ---------------------------------------------------------------------------------------*/
/* McuLittleFS */
#define LITTLEFS_CONFIG_ENABLED                       (1) /* enable the LittleFS file system */

/* -------------------------------------------------*/
/* McuW25Q128 */
#define MCUW25Q128_CONFIG_ENABLED               (1)
#define MCUW25Q128_CONFIG_SIZE_KBYTES           (16*1024) /* we have a 16 MByte device */
/* -------------------------------------------------*/
/* McuLittleFS */
#define LITTLEFS_CONFIG_ENABLED                       (1)
#define McuLittleFSBlockDevice_CONFIG_MEMORY_TYPE     McuLittleFSBlockDevice_CONFIG_MEMORY_TYPE_WINBOND_W25Q128
#define McuLittleFS_CONFIG_BLOCK_SIZE                 (4096) /* W25Q128 has blocks of 4 KByte */
#define McuLittleFS_CONFIG_BLOCK_COUNT                ((MCUW25Q128_CONFIG_SIZE_KBYTES*1024)/McuLittleFS_CONFIG_BLOCK_SIZE) /* W25Q128 has 16 MByte */
#define McuLittleFS_CONFIG_BLOCK_OFFSET               (0)
/* ---------------------------------------------------------------------------------------*/

#endif /* MCULIB_CONFIG_CONFIG_H_ */
