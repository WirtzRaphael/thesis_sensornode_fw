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

#define UART_RADIO_ID UART0_ID
#define UART_DELAY (50)



void radio_init(){
    uart_init(UART_RADIO_ID, 9600);
    gpio_set_function(RADIO_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(RADIO_PIN_RX, GPIO_FUNC_UART);

    gpio_init(RADIO_PIN_CONFIG);
    gpio_set_dir(RADIO_PIN_CONFIG, GPIO_OUT);
    gpio_put(RADIO_PIN_CONFIG, true);
}

void radio_read_temperature() {
        
    gpio_put(RADIO_PIN_CONFIG, false);

    uint8_t buffer[10];
    for (int i = 0; i < 4; i++) {
        if(uart_is_readable(UART_RADIO_ID)){
            //char ch = uart_getc(UART_RADIO_ID);          
            uart_read_blocking(UART_RADIO_ID, buffer, 10);
            McuLog_trace("Received !");
            //McuLog_trace("Received %c from radio", ch);
            McuLog_trace("Received %s from radio", buffer);
            break;
        } else {
            McuLog_trace("Wait radio receive");
            sleep_ms(500);
        }       
    }
    gpio_put(RADIO_PIN_CONFIG, true);
    /*
    uart_puts(UART_RADIO_ID, "X");
    McuLog_trace("Sent X to radio");
    sleep_ms(UART_DELAY);
    uart_puts(UART_RADIO_ID, "U");
    McuLog_trace("Sent U to radio");
    sleep_ms(UART_DELAY);
    */
    /*
    McuLog_trace("Wait for response");
    uint8_t buffer[10];
    uart_read_blocking(UART_RADIO_ID, buffer, 10);
    for (int i = 0; i < 10; i++) {
        printf("%c", buffer[i]);
    }
    McuLog_trace("Received response");
    */
   McuLog_trace("Finished read temperature");
}
