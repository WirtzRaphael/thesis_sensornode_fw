#ifndef PICO_CONFIG_H_
#define PICO_CONFIG_H_

extern uint32_t SystemCoreClock;

#define MODEL_PICO (0)
#define MODEL_PICO_W (1)

#define HW_SENSORNODE_V1 (1)

/* UART
*/
#define UART1_ID uart1
#define UART1_BAUD_RATE 19200

/* Pins
*/
#if MODEL_PICO
    static const uint LED_PIN = PICO_DEFAULT_LED_PIN;
#endif
#if MODEL_PICO_W
    //static const uint LED_PIN = CYW43_WL_GPIO_LED_PIN
#endif
#define PICO_PINS_I2C0_SDA 16
#define PICO_PINS_I2C0_SCL 17
#define PICO_PINS_UART1_TX 4
#define PICO_PINS_UART1_RX 5

/* Features
*/
#define PICO_CONFIG_USE_TMP117 (0)
#define PICO_CONFIG_USE_SLEEP (0)
#define PICO_CONFIG_USE_DISPLAY (0)
#define PICO_CONFIG_USE_RADIO (0)
#define PICO_CONFIG_USE_HEARTBEAT (1)

/* PL : sensornode v1 - rp2040 pico w
*/
#if MODEL_PICO_W && HW_SENSORNODE_V1
    #define PL_TEST_OUTPUT 29
#endif

/* PL : AEMBS Board
*/
#define PL_AEMBS_CONFIG_USE_BUTTONS 0
#define PL_AEMBS_SWITCH_MIDDLE 15
#define PL_AEMBS_SWITCH_RIGHT 16
#define PL_AEMBS_SWITCH_LEFT 17
#define PL_AEMBS_SWITCH_DOWN 19
#define PL_AEMBS_SWITCH_UP 20

#define PL_AEMBS_LED_BLUE 18
#define PL_AEMBS_LED_GREEN 19
#define PL_AEMBS_LED_RED 20

#endif /* PICO_CONFIG_H_ */