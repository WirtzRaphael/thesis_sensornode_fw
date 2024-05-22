#ifndef PICO_CONFIG_H_
#define PICO_CONFIG_H_
#include "IncludeMcuLibConfig.h"

#include "stdint.h"
#include <stdint.h>
extern uint32_t SystemCoreClock;

#define MODEL_PICO   (0)
#define MODEL_PICO_W (0)
#define MODEL_RP2040 (1)

// #define HW_PLATFORM (PL_CONFIG_HW_VERSION_1_0)
#define HW_PLATFORM (PL_CONFIG_HW_VERSION_2_0)

/* UART
 */
#define UART0_ID        uart0
#define UART0_BAUD_RATE 19200
#define UART0_CTS       1
#define UART0_RTS       0

#define UART1_ID        uart1
#define UART1_BAUD_RATE 19200

/* Pins
 */
#if MODEL_PICO
static const uint LED_PIN = PICO_DEFAULT_LED_PIN;
#endif
#if MODEL_PICO_W
  // static const uint LED_PIN = CYW43_WL_GPIO_LED_PIN
#endif

/* Features
 */
// todo : rename general
// #define PICO_CONFIG_USE_TMP117    (0)
#define PICO_CONFIG_USE_SLEEP     (0)
#define PICO_CONFIG_USE_DISPLAY   (0)
#define PICO_CONFIG_USE_HEARTBEAT (0)

#if MODEL_RP2040 && HW_PLATFORM == PL_CONFIG_HW_VERSION_2_0
  /* Features
   */
  #define PICO_CONFIG_USE_RTC     (1)
  #define PICO_CONFIG_USE_MENU    (1)
  #define PICO_CONFIG_USE_BUTTONS (1)
  #define PICO_CONFIG_USE_RADIO   (1)
  #define PICO_CONFIG_USE_SENSORS (1)
  // todo : energy/power

  /* GPIO
   */
  #define PICO_PINS_BUTTON_A    14u
  #define PICO_PINS_BUTTON_B    15u
  #define PICO_PINS_I2C0_SDA    0u
  #define PICO_PINS_I2C0_SCL    1u
  #define PICO_PINS_I2C0_ENABLE 2u
  #define PICO_PINS_I2C1_SDA    18u
  #define PICO_PINS_I2C1_SCL    19u
  #define PL_GPIO_RADIO_RESET   20u
  #define PL_GPIO_RADIO_CONFIG  21u
  // UART 0 : Radio
  #define PICO_PINS_UART0_RX  25u
  #define PICO_PINS_UART0_TX  24u
  #define PICO_PINS_UART0_CTS 22u
  #define PICO_PINS_UART0_RTS 23u
  // todo : rs232
  // todo : LED's
  // todo : gpio power -> Fix connection
#endif

/* PL : sensornode v1 - rp2040 pico w
 */
#if MODEL_PICO_W && HW_PLATFORM == PL_CONFIG_HW_VERSION_1_0
  /* Features
   */
  #define PICO_CONFIG_USE_TMP117    (0)
  #define PICO_CONFIG_USE_SLEEP     (0)
  #define PICO_CONFIG_USE_DISPLAY   (0)
  #define PICO_CONFIG_USE_RADIO     (0)
  #define PICO_CONFIG_USE_HEARTBEAT (0)

  /* GPIO
   */
  #define PICO_PINS_BUTTON_A 0u
  #define PICO_PINS_BUTTON_B 1u
  #define PICO_PINS_BUTTON_C 2u
  #define PICO_PINS_I2C0_SDA 8
  #define PICO_PINS_I2C0_SCL 9
  #define PICO_PINS_I2C1_SDA 6
  #define PICO_PINS_I2C1_SCL 7
  #define PL_GPIO_RADIO_CONFIG  20
  // UART 0 : Radio
  #define PICO_PINS_UART0_RX  17
  #define PICO_PINS_UART0_TX  16
  #define PICO_PINS_UART0_CTS 19
  #define PICO_PINS_UART0_RTS 18
  // UART 1 : RS232
  // #define PICO_PINS_UART1_TX 4
  // #define PICO_PINS_UART1_RX 5
  //
  #define PL_GPIO_DISPLAY_ENABLE 3
  #define PL_GPIO_ENABLE_VCC_RF  28
  #define PL_GPIO_RADIO_RESET    26
  #define PL_GPIO_TEST_OUTPUT    29
#endif

/* PL : AEMBS Board
 */
#if MODEL_PICO && HW_PLATFORM == PL_CONFIG_HW_AEMBS_BOARD
  /* Features
   */
  #define PICO_CONFIG_USE_TMP117    (0)
  #define PICO_CONFIG_USE_SLEEP     (0)
  #define PICO_CONFIG_USE_DISPLAY   (0)
  #define PICO_CONFIG_USE_RADIO     (0)
  #define PICO_CONFIG_USE_HEARTBEAT (0)

  /* GPIO
   */
  #define PL_AEMBS_CONFIG_USE_BUTTONS 0
  #define PL_AEMBS_SWITCH_MIDDLE      15
  #define PL_AEMBS_SWITCH_RIGHT       16
  #define PL_AEMBS_SWITCH_LEFT        17
  #define PL_AEMBS_SWITCH_DOWN        19
  #define PL_AEMBS_SWITCH_UP          20
  #define LED_PIN                     (25) /* GPIO 25 */

  #define PL_AEMBS_LED_BLUE  18
  #define PL_AEMBS_LED_GREEN 19
  #define PL_AEMBS_LED_RED   20
#endif

#endif /* PICO_CONFIG_H_ */