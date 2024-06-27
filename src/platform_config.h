#ifndef PLATFORM_CONFIG_H_
#define PLATFORM_CONFIG_H_
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
#define UART0_CTS       0
#define UART0_RTS       0

#define UART1_ID        uart1
#define UART1_BAUD_RATE 19200
#define UART1_CTS       0
#define UART1_RTS       0

/* Pins
 */
#if MODEL_PICO
static const uint LED_PIN = PICO_DEFAULT_LED_PIN;
#endif
#if MODEL_PICO_W
  // static const uint LED_PIN = CYW43_WL_GPIO_LED_PIN
#endif

#if MODEL_RP2040 && HW_PLATFORM == PL_CONFIG_HW_VERSION_2_0
  /** Features
   */
  #define PLATFORM_CONFIG_USE_RTC     (1)
  #define PLATFORM_CONFIG_USE_MENU    (1)
  #define PLATFORM_CONFIG_USE_BUTTONS (1)
  // hint : deactivate when no VCC_RF
  #define PLATFORM_CONFIG_USE_RADIO                                            \
    (0 || APP_DEMO_AUTOSHUTDOWN_WITH_RADIO ||                                  \
     APP_DEMO_NO_AUTOSHUTDOWN_WITH_RADIO)
  #define PLATFORM_CONFIG_USE_SENSORS (1)
  #define PLATFORM_CONFIG_USE_POWER   (1)
  //
  // fixme : working from menu. otherwise? Clear Buffer?
  #define APP_HAS_ONBOARD_GREEN_LED (0)
  /** RADIO
   */
  #define APP_RADIO_DECTIVATE_RF                                               \
    (0 || APP_DEMO_AUTOSHUTDOWN_WITH_RADIO ||                                  \
     APP_DEMO_NO_AUTOSHUTDOWN_WITH_RADIO)
  #define APP_RADIO_CHANNEL_SCAN (0)
  /** POWER
   */
  #define APP_POWER_RADIO_DEFAULT_SLEEP (1)
  /**  AUTO-SHUTDOWN
   * - mcu shutdowns own voltage supply
   * - use rtc to wakeup circuit
   * - restart via watchdog if voltage supply not shutdown
   */
  #define APP_POWER_AUTO_SHUTDOWN                                              \
    (0 || APP_DEMO_AUTOSHUTDOWN_WITH_RADIO || APP_DEMO_AUTOSHUTDOWN_NO_RADIO)
  #define APP_POWER_APP_TASK_MS        (1000)
  #define APP_POWER_WAKEUP_FALLBACK_MS (4000)
  // fixme : delay  deinit i2c etc.
  #define APP_POWER_DEINIT_MS (50)
  /* RTC
   */
  #define APP_RTC_ALERT_DELTA_SEC (5)
  /* WATCHDOG
   */
  #define PL_CONFIG_USE_WATCHDOG_PICO (0)
  /** DEMO : AUTOSHUTDOWN NO RADIO
   * HW requirements
   * - VCC_RF disconnected
   */
  #define APP_DEMO_AUTOSHUTDOWN_NO_RADIO (0)
  /** DEMO : NO AUTOSHUTDOWN WITH RADIO
   * HW requirements
   * - VCC_RF connected
   */
  // todo [demo] : periodic send
  #define APP_DEMO_NO_AUTOSHUTDOWN_WITH_RADIO (1)
  /** DEMO : AUTOSHUTDOWN WITH RADIO
   */
  // todo [demo] : APP shutdown sync/fix time
  // todo [demo] : periodic send (based on time sync rtc)
  // todo [demo] : NVM memory -> info for radio send (eg. last send time)
  // todo [demo] : MODE TEST
  #define APP_DEMO_AUTOSHUTDOWN_WITH_RADIO (0)
  /* GPIO
   */
  #define PICO_PINS_BUTTON_A    14u
  #define PICO_PINS_BUTTON_B    15u
  #define PICO_PINS_LED_1       17u
  #define PICO_PINS_LED_2       16u
  #define PICO_PINS_I2C0_SDA    0u
  #define PICO_PINS_I2C0_SCL    1u
  #define PICO_PINS_I2C0_ENABLE 2u
  #define PICO_PINS_I2C1_SDA    18u
  #define PICO_PINS_I2C1_SCL    19u
  #define PL_GPIO_RADIO_RESET   20u
  #define PL_GPIO_RADIO_CONFIG  21u
  // UART 0 : Radio
  #define PICO_PINS_UART1_RX  25u
  #define PICO_PINS_UART1_TX  24u
  #define PICO_PINS_UART1_CTS 22u
  #define PICO_PINS_UART1_RTS 23u
  // todo : rs232 : uart0
  #define PICO_PINS_RS232_FORCEOFF_N 9u
  #define PICO_PINS_RS232_FORCEON    10u
  // todo : replace in code
  // fix : hardware modification
  // #define PICO_PINS_POWER_3V3_1_ENABLE NULL
  // #define PICO_PINS_POWER_3V3_2_ENABLE NULL
  // #define PICO_PINS_POWER_3V3_MODE NULL
#endif

/* PL : sensornode v1 - rp2040 pico w
 */
#if MODEL_PICO_W && HW_PLATFORM == PL_CONFIG_HW_VERSION_1_0
  /* Features
   */
  #define PLATFORM_CONFIG_USE_TMP117    (0)
  #define PLATFORM_CONFIG_USE_SLEEP     (0)
  #define PLATFORM_CONFIG_USE_DISPLAY   (0)
  #define PLATFORM_CONFIG_USE_RADIO     (0)
  #define PLATFORM_CONFIG_USE_HEARTBEAT (0)
  #define PLATFORM_CONFIG_USE_SLEEP     (0)

  /* GPIO
   */
  #define PICO_PINS_BUTTON_A   0u
  #define PICO_PINS_BUTTON_B   1u
  #define PICO_PINS_BUTTON_C   2u
  #define PICO_PINS_I2C0_SDA   8
  #define PICO_PINS_I2C0_SCL   9
  #define PICO_PINS_I2C1_SDA   6
  #define PICO_PINS_I2C1_SCL   7
  #define PL_GPIO_RADIO_CONFIG 20
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
  #define PLATFORM_CONFIG_USE_TMP117    (0)
  #define PLATFORM_CONFIG_USE_SLEEP     (0)
  #define PLATFORM_CONFIG_USE_DISPLAY   (0)
  #define PLATFORM_CONFIG_USE_RADIO     (0)
  #define PLATFORM_CONFIG_USE_HEARTBEAT (0)
  #define PLATFORM_CONFIG_USE_SLEEP     (0)

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

#endif /* PLATFORM_CONFIG_H_ */