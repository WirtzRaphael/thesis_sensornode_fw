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
#define UART_BAUD_RATE UART0_BAUD_RATE
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
