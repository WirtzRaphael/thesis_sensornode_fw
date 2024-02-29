#ifndef PICO_CONFIG_H_
#define PICO_CONFIG_H_

extern uint32_t SystemCoreClock;

/* UART
*/
#define UART1_ID uart1
#define UART1_BAUD_RATE 19200

/* Pins
*/
static const uint LED_PIN = PICO_DEFAULT_LED_PIN;
#define PICO_PINS_I2C0_SDA 16
#define PICO_PINS_I2C0_SCL 17
#define PICO_PINS_UART1_TX 4
#define PICO_PINS_UART1_RX 5

/* Features
*/
#define PICO_CONFIG_USE_TMP117 (1)
#define PICO_CONFIG_USE_SLEEP (0)
#define PICO_CONFIG_USE_DISPLAY (1)
#define PICO_CONFIG_USE_RADIO (1)
#define PICO_CONFIG_USE_HEARTBEAT (1)


/* PL : AEMBS Board
*/
#define PL_CONFIG_USE_BUTTONS 0
#define PL_SWITCH_MIDDLE 15
#define PL_SWITCH_RIGHT 16
#define PL_SWITCH_LEFT 17
#define PL_SWITCH_DOWN 19
#define PL_SWITCH_UP 20

#define PL_LED_BLUE 18
#define PL_LED_GREEN 19
#define PL_LED_RED 20

#endif /* PICO_CONFIG_H_ */