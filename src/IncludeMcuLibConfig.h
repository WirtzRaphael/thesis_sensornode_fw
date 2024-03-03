/*
 * Copyright (c) 2023, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* header file is included with -include compiler option */

#ifndef MCULIB_CONFIG_CONFIG_H_
#define MCULIB_CONFIG_CONFIG_H_

/* ---------------------------------------------------------------------------------------*/
/* SDK */
#define McuLib_CONFIG_CPU_IS_KINETIS                (0)
#define McuLib_CONFIG_CORTEX_M                      (0) /* RP2040 is a Cortex-M0+ */
#define McuLib_CONFIG_CPU_IS_RPxxxx                 (1)
#define McuLib_CONFIG_CPU_VARIANT                   McuLib_CONFIG_CPU_VARIANT_RP2040
#define McuLib_CONFIG_SDK_VERSION_USED              McuLib_CONFIG_SDK_RPI_PICO
/* ---------------------------------------------------------------------- */
/* FreeRTOS */
#define McuLib_CONFIG_SDK_USE_FREERTOS              (0)
#define configMINIMAL_STACK_SIZE                    (500/sizeof(StackType_t))
#define configTIMER_TASK_STACK_DEPTH                (800/sizeof(StackType_t)) /* stack size for Timer Service task */
#define configTOTAL_HEAP_SIZE                       (64*1024)
#define configUSE_SEGGER_SYSTEM_VIEWER_HOOKS        (0)
/* -------------------------------------------------*/
/* I2C */
#define CONFIG_USE_HW_I2C                           (1) /* if using HW I2C, otherwise use software bit banging */
#define MCUI2CLIB_CONFIG_I2C_DEVICE                 i2c0
#define MCUI2CLIB_CONFIG_SDA_GPIO_PIN               8u
#define MCUI2CLIB_CONFIG_SCL_GPIO_PIN               9u
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
/* ------------------- McuLittleFS --------------------------*/
#define LITTLEFS_CONFIG_ENABLED                       (0) /* enable the LittleFS file system */
#if 0 /* using Winbond external flash */
  #define McuLittleFSBlockDevice_CONFIG_MEMORY_TYPE     McuLittleFSBlockDevice_CONFIG_MEMORY_TYPE_WINBOND_W25Q128
#else /* using internal flash with McuFlash */
  #define McuLittleFSBlockDevice_CONFIG_MEMORY_TYPE     McuLittleFSBlockDevice_CONFIG_MEMORY_TYPE_MCU_FLASH
  #define McuLittleFS_CONFIG_BLOCK_SIZE                 (McuFlash_CONFIG_FLASH_BLOCK_SIZE)
  #define McuLittleFS_CONFIG_BLOCK_COUNT                (McuFlash_CONFIG_NOF_BLOCKS)
  #define McuLittleFS_CONFIG_BLOCK_OFFSET               ((McuFlash_CONFIG_MEM_START)/(McuFlash_CONFIG_FLASH_BLOCK_SIZE))
#endif
/* ---------------------------------------------------------------------------------------*/

#endif /* MCULIB_CONFIG_CONFIG_H_ */
