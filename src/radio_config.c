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

static void enter_config_mode(void) {
    McuLog_trace("Enter config mode");
    #if LOG_LEVEL_DEBUG
    McuLog_debug("Config pin low");
    #endif
    gpio_put(RADIO_PIN_CONFIG, false);
    sleep_ms(50);
}


static void exit_config_mode(void) {
    McuLog_trace("Exit config mode");
    #if LOG_LEVEL_DEBUG
    McuLog_debug("Config pin high");
    McuLog_debug("Send X to radio");
    #endif
    gpio_put(RADIO_PIN_CONFIG, true);
    sleep_ms(50);

    uart_puts(UART_RADIO_ID, "X");
    uart_wait();
}

void radio_init(){
    //uart_init(UART_RADIO_ID, UART_BAUD_RATE);
    uart_init(UART_RADIO_ID, 19200);
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

void radio_read_temperature(void) {
    bool continueConfig = false;

    // be sure to not be already in config mode
    exit_config_mode();

    enter_config_mode();

    // -- Send command 
    //uart_write_blocking(UART_ID, &pre, 1);
    uart_puts(UART_RADIO_ID, "U");
    McuLog_trace("Send U to radio");
    uart_wait();

    uint8_t buffer_size = 3;
    uint8_t rec_buffer[buffer_size];
    uart_read_blocking(UART_RADIO_ID, rec_buffer, buffer_size);
    McuLog_trace("Received %d from radio\n", rec_buffer[0]);
    McuLog_trace("Received %d from radio\n", rec_buffer[1]);
    McuLog_trace("Received %d from radio\n", rec_buffer[2]);

    // check if the buffer contains '>'
    // save the position of the temperature
    uint8_t count = 0;
    uint8_t pos = 10;
    for (uint8_t i = 0; i < buffer_size; i++) {
        if (rec_buffer[i] == 62) {
            count++;
        } else if (rec_buffer[i] != 0) {
            pos = i;
        }
    }

    // calculate temperature if the buffer contains '>' and not 0
    if (count >= 1 && pos != 10) {
        uint8_t temperature = rec_buffer[pos] - 128;
        McuLog_trace("Temperature is : %d", temperature);
        #if PRINTF
        printf("Temperature is : %d\n", temperature);
        #endif
    } else{
        McuLog_trace("Response invalid");
    }

    exit_config_mode();
    McuLog_trace("Finished read temperature");
}

