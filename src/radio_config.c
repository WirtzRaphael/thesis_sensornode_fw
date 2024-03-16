#include <stdio.h>
#include "stdio.h"

#include "pico_config.h"
#include "pico/stdlib.h"

#include "McuUtility.h"
#include "McuLog.h"
#include "McuWait.h"

#include "hardware/uart.h"

#define RADIO_PIN_TX PICO_PINS_UART0_TX
#define RADIO_PIN_RX PICO_PINS_UART0_RX
#define RADIO_PIN_CONFIG (20)
#define RADIO_HW_FLOW_CONTROL (0)

#define UART_RADIO_ID UART0_ID
//#define UART_BAUD_RATE UART0_BAUD_RATE
#define UART_BAUD_RATE 19200
#define UART_DELAY (50)

#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

static void uart_wait(void) {
    sleep_ms(50); // todo : decrease
}

static void send_payload_separator(void) {
    uart_puts(UART_RADIO_ID, payload_separator_char);
    uart_wait();
}

static void send_exit_config(void) {
    gpio_put(RADIO_PIN_CONFIG, true);
    sleep_ms(50);
    uart_puts(UART_RADIO_ID, "X");
    uart_wait();
    McuLog_trace("Exit config mode with X");
}

void radio_init(){
    uart_init(UART_RADIO_ID, 9600);
    gpio_set_function(RADIO_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(RADIO_PIN_RX, GPIO_FUNC_UART);

    // Enable VCC_RF
    gpio_init(PL_GPIO_ENABLE_VCC_RF);
    gpio_set_dir(PL_GPIO_ENABLE_VCC_RF, GPIO_OUT);
    gpio_put(PL_GPIO_ENABLE_VCC_RF, true);

    // Reset Pin
    gpio_init(PL_GPIO_RADIO_RESET);
    gpio_set_dir(PL_GPIO_RADIO_RESET, GPIO_OUT);
    gpio_put(PL_GPIO_RADIO_RESET, true);

    // HW flow control (default)
#if RADIO_HW_FLOW_CONTROL
    uart_set_hw_flow(UART_RADIO_ID, true, true);
#else
    uart_set_hw_flow(UART_RADIO_ID, false, false);
#endif

    uart_set_format(UART_RADIO_ID, DATA_BITS, STOP_BITS, PARITY);
    

    gpio_init(RADIO_PIN_CONFIG);
    gpio_set_dir(RADIO_PIN_CONFIG, GPIO_OUT);
    gpio_put(RADIO_PIN_CONFIG, true);
}

void radio_reset(void) {
    McuLog_trace("Reset radio");
    gpio_put(PL_GPIO_RADIO_RESET, false);
    sleep_ms(100);
    gpio_put(PL_GPIO_RADIO_RESET, true);
}

void radio_send(void) {
    uart_putc_raw(UART_RADIO_ID, 21);
    uart_wait();
    send_payload_separator();
    //printf("send: %s \r\n", strId);
    uart_putc_raw(UART_RADIO_ID, 5);
    uart_wait();
}

    gpio_put(RADIO_PIN_CONFIG, false);

    bool continueConfig = false;
    // receive '>'
    for (int i = 0; i < 5; i++) {
        sleep_ms(200);
        if(uart_is_readable(UART_RADIO_ID)){
            McuLog_trace("UART is readable");
            //char ch = uart_getc(UART_RADIO_ID);          
            //printf("Received %c from radio", ch);
            continueConfig = true;
            break;
        }
            McuLog_trace("Wait radio receive");
    }



    gpio_put(RADIO_PIN_CONFIG, true);
   McuLog_trace("Finished read temperature");
}
