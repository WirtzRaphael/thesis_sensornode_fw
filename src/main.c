#include <stdio.h>
#include <string.h>

/* Pico
*/
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "pico/unique_id.h"

#include "hardware/i2c.h"
#include "hardware/timer.h" // todo move
#include "hardware/uart.h"
// fixme : link to board
#if MODEL_PICO
#include "../pico-sdk/src/boards/include/boards/pico.h"
#endif
#if MODEL_PICO_W
    //#include "../pico-sdk/src/boards/include/boards/pico_w.h"
    #include "pico/cyw43_arch.h"
#endif
#include "pico/util/queue.h"

/* McuLib
*/
#include "McuWait.h"
// #include "McuLib.h"
//#include "McuLED.h"
// todo : cleanup
/*
#include "McuShellUart.h"
#include "McuShell.h"
#include "McuRTT.h"
#include "McuSystemView.h"
#include "McuLog.h"
#include "McuTimeDate.h"
#include "McuDebounce.h"
#include "McuSWO.h"
*/

/* project files
*/
#include "platform.h"
#include "low_power_operations.h"
#include "i2c_operations.h"
#include "display.h"
#include "tmp117.h"
#include "time_operations.h"
#include "radio.h"
#include "sensors.h"

//#include "littleFS/McuLittleFS.h"

/* platform
*/
pico_unique_board_id_t pico_unique_board_id;

// todo : configurable or auto generated
#define RADIO_UID 10 // unique id

/* globals
*/
uint32_t SystemCoreClock = 120000000;

/* Clocks
*/


/* State machine
*/
typedef enum {
    FSM_STATE_START,
    FSM_STATE_HEARTBEAT,
    FSM_STATE_SENSORS_TEMPERATURE,
    FSM_STATE_DISPLAY,
    FSM_STATE_RADIO,
    FSM_STATE_LOW_POWER_SLEEP,
    FSM_STATE_END
}fsm_state_t;

typedef enum {
    FSM_EVENT_NONE,
    FSM_EVENT_SENSORS_TEMPERATURE_READ,
    FSM_EVENT_HEARTBEAT_END,
    FSM_EVENT_DISPLAY_SHOW,
    FSM_EVENT_DISPLAY_SLEEP,
    FSM_EVENT_LOW_POWER_WAKEUP,
    FSM_EVENT_RADIO_IDLE,
    FSM_EVENT_RADIO_SEND
}fsm_event_t;

typedef struct {
    fsm_state_t state;
    /*
    fsm_event event;
    fsm_state next_state;
    */
}fsm_t;

/* Sensors
*/
// Sensor values
// fixme : syntax, remove ?
sensor_temp_t temperatureSensor1 = {0,0, 0};
sensor_temp_t temperatureSensor2 = {0,0, 0};

// Queue of sensor values
queue_t temperatureSensor1_queue;
queue_t temperatureSensor2_queue;
const int QUEUE_LENGTH = 128;

// Time series of sensor values
time_series_sensor_t temperatureSensor1_time_series = {
    .sensor_nr = '1',
    .time_reference = 0,
    .queue = &temperatureSensor1_queue
};

time_series_sensor_t temperatureSensor2_time_series = {
    .sensor_nr = '2',
    .time_reference = 0,
    .queue = &temperatureSensor2_queue
};

/* Low power
*/

/* Radio
*/
int radio_counter = 0;
#define RADIO_SEND_INTERVAL 2

/*******************************************************************************
 * Function Declarations
 */
static bool heartbeat = true;
void heartbeat_status(void);
//void update_temperature_sensor(i2c_inst_t *i2c, const uint i2c_addr, queue_t *temperatureSensor_queue);

/*******************************************************************************
 * Function Definitions
 */
fsm_event_t state_heartbeat_handler(void) {
#if PICO_CONFIG_USE_HEARTBEAT
    printf("HANDLER: HEARBEAT\r\n");
    heartbeat_status();
    //fsm->state = SENSORS_TEMPERATURE;
#endif // PICO_CONFIG_USE_HEARTBEAT

    return FSM_EVENT_HEARTBEAT_END;
}

fsm_event_t state_sensors_temperature_handler(i2c_inst_t *i2c) {
#if PICO_CONFIG_USE_TMP117
    printf("HANDLER: TMP117\r\n");

    /* TMP117 1
    */
    // Read device ID to make sure that we can communicate with the ADXL343
    uint16_t data1_16, data2_16;
    uint16_t id1, id2;

    id1 = tmp117_read_id(i2c, TMP117_1_ADDR);

    printf("TMP117 1: Sensor ID %d\r\n", id1);
    if(id1 != 0) {
        data1_16 = tmp117_read_temperature(i2c, TMP117_1_ADDR);
        temperatureSensor1.temperature = tmp117_temperature_to_celsius(data1_16);
        temperatureSensor1.id += 1;
        temperatureSensor1.time_relative_to_reference = time_operations_get_time_diff_s(
            temperatureSensor1_time_series.time_reference, time_operations_get_time_us());
        if (queue_try_add(&temperatureSensor1_queue, &temperatureSensor1)) {
            printf("QUEUE add success\r\n");
        }
        printf("TMP117 1, Temp %f\r\n", temperatureSensor1.temperature);
        printf("TMP117 1, Data %d\r\n", data1_16);
    } else {
        printf("ERROR: Could not communicate with TMP117 1\r\n");
    }

    /* TMP117 2 */
    id2 = tmp117_read_id(i2c, TMP117_2_ADDR);
    printf("TMP117 2: Sensor ID %d\r\n", id2);
    if (id2 != 0) {
        data2_16 = tmp117_read_temperature(i2c, TMP117_2_ADDR);
        temperatureSensor2.temperature = tmp117_temperature_to_celsius(data2_16);
        temperatureSensor2.id += 1;
        temperatureSensor2.time_relative_to_reference = time_operations_get_time_diff_s(
            temperatureSensor2_time_series.time_reference, time_operations_get_time_us());
        if (queue_try_add(&temperatureSensor2_queue, &temperatureSensor2)) {
            printf("QUEUE add success\r\n");
        }
        printf("TMP117 2, Temp %f\r\n", temperatureSensor2.temperature);
        printf("TMP117 2, Data %d\r\n", data2_16);
    } else {
        printf("ERROR: Could not communicate with TMP117 2\r\n");
    }
    /* DEBUG
    //display_status(data16, data2_16);
    //display_status(temperatureSensor1, temperatureSensor2);
    */
#endif // PICO_CONFIG_USE_TMP117

    return FSM_EVENT_SENSORS_TEMPERATURE_READ;
}

fsm_event_t state_display_handler(void) {
#if PICO_CONFIG_USE_DISPLAY
    display_status_float(get_latest_temperature(temperatureSensor1_queue), get_latest_temperature(temperatureSensor2_queue));   
#endif // PICO_CONFIG_USE_DISPLAY

    return FSM_EVENT_DISPLAY_SHOW;
}

fsm_event_t state_low_power_sleep_handler(void) {
#if PICO_CONFIG_USE_SLEEP
    low_power_sleep();
#else
    sleep_ms(5000);
#endif // PICO_CONFIG_USE_SLEEP

    return FSM_EVENT_LOW_POWER_WAKEUP;
}

fsm_event_t state_radio_handler(void) {
#if PICO_CONFIG_USE_RADIO
    printf("HANDLER: RADIO\r\n");
    if (radio_counter >= RADIO_SEND_INTERVAL) { // todo : define
        radio_counter = 0;
            gpio_put(PL_LED_BLUE, true);
            //radio_send_test_messages();
            radio_send_sensor_temperature_series(temperatureSensor1_time_series);
            sleep_ms(100);
            radio_send_sensor_temperature_series(temperatureSensor2_time_series);
            gpio_put(PL_LED_BLUE, false);
            //return FSM_EVENT_RADIO_SEND;
            return FSM_EVENT_RADIO_IDLE;
    } else {
        radio_counter++;
        return FSM_EVENT_RADIO_IDLE;
    }

#endif

    return FSM_EVENT_RADIO_IDLE;
}


void handleEvent(fsm_t *fsm, fsm_event_t event) {
    switch (fsm->state) {
        case FSM_STATE_HEARTBEAT:
            printf("STATE: Heartbeat\n");
            if (event == FSM_EVENT_HEARTBEAT_END) {
                fsm->state = FSM_STATE_SENSORS_TEMPERATURE;
            }
            break;
        case FSM_STATE_SENSORS_TEMPERATURE:
            printf("STATE: Sensor temperature \n");
            if (event == FSM_EVENT_SENSORS_TEMPERATURE_READ) {
                fsm->state = FSM_STATE_DISPLAY;
            }
            break;

        case FSM_STATE_DISPLAY:
            printf("STATE: Display\n");
            if (event == FSM_EVENT_DISPLAY_SHOW) {
                fsm->state = FSM_STATE_RADIO;
            }
            break;
        
        case FSM_STATE_RADIO:
            printf("STATE: Radio\n");
            if (event == FSM_EVENT_RADIO_IDLE) {
                fsm->state = FSM_STATE_LOW_POWER_SLEEP;
            }
            if (event == FSM_EVENT_RADIO_SEND) {
                fsm->state = FSM_STATE_LOW_POWER_SLEEP;
            }
            break;


        case FSM_STATE_LOW_POWER_SLEEP:
            printf("STATE: Low power sleep\n");
            sleep_ms(200); //  fixme : remove 
            event = FSM_EVENT_LOW_POWER_WAKEUP;
            if (event == FSM_EVENT_LOW_POWER_WAKEUP) {
                fsm->state = FSM_STATE_SENSORS_TEMPERATURE;
            }
            break;
    }
}

/*******************************************************************************
 * main
 */
int main(void)
{
    /*******************************************************************************
     * Initialization
    */
    /* Platform
    */
    pico_get_unique_board_id(&pico_unique_board_id);

    /* Serial port
    */
    // Initialize chosen serial port
    stdio_init_all();
    if (cyw43_arch_init()) {
        return -1;
    }

    printf("Starting\n");

    low_power_init();

    /* GPIO
    */

#if MODEL_PICO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
#if MODEL_PICO_W
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
#endif

    // Voltage output for sensors
    gpio_init(PL_LED_GREEN);
    gpio_set_dir(PL_LED_GREEN, GPIO_OUT);
    gpio_put(PL_LED_GREEN, true);

    gpio_init(PL_LED_BLUE);
    gpio_set_dir(PL_LED_BLUE, GPIO_OUT);
    gpio_init(PL_LED_RED);
    gpio_set_dir(PL_LED_RED, GPIO_OUT);

    /* McuLib
    */
    McuWait_Init(); /*Init MCU Wait*/

    /* Rtc sleep
    */
    // Start the Real time clock
    rtc_init();

    clock_measure_freqs();
    uart_default_tx_wait_blocking();
    
    /* I2C
    */
    //McuGenericI2C_Init();
    //McuI2cLib_Init();
    
    i2c_operations_init(PICO_PINS_I2C0_SDA, PICO_PINS_I2C0_SCL);
    // Initialize I2C0 port at 400 kHz
    i2c_inst_t *i2c_0 = i2c0;
    i2c_init(i2c_0, 400 * 1000);

    /* UART
    */
    uart_init(UART1_ID,UART1_BAUD_RATE);
    gpio_set_function(PICO_PINS_UART1_TX, GPIO_FUNC_UART);
    gpio_set_function(PICO_PINS_UART1_RX, GPIO_FUNC_UART);
    // todo hw flow control
    // uart_set_hw_flow
    
    
    /* Sensors
    */
    queue_init(&temperatureSensor1_queue, sizeof(sensor_temp_t), QUEUE_LENGTH);
    queue_init(&temperatureSensor2_queue, sizeof(sensor_temp_t), QUEUE_LENGTH);

    temperatureSensor1_time_series.time_reference = time_operations_get_time_us();
    temperatureSensor2_time_series.time_reference = time_operations_get_time_us();

    /* Display
    */
    display_init();


    while (true)
    {
        /* State machine
         */
        fsm_t fsm = {FSM_STATE_HEARTBEAT};
        fsm_event_t event = FSM_EVENT_NONE;
        
        /*
         * trigger state transitions
         */
        event = state_heartbeat_handler();
        handleEvent(&fsm, event);
        sleep_ms(10);

        /* Sensors temperature
        */
        //handleEvent(&fsm, HEARTBEAT_END);
#if PICO_CONFIG_USE_TMP117
        event = state_sensors_temperature_handler(i2c_0);
        handleEvent(&fsm, event);
#else
        handleEvent(&fsm, FSM_EVENT_SENSORS_TEMPERATURE_READ);
#endif // PICO_CONFIG_USE_TMP117
        sleep_ms(10);

        /* Display
        */
        event = state_display_handler();
        handleEvent(&fsm, event);
        sleep_ms(10);     
        
        /* RADIO
        */
        event = state_radio_handler();
        handleEvent(&fsm, event);
        sleep_ms(10);

        /* Low Power
        */
        handleEvent(&fsm, FSM_EVENT_NONE);
        sleep_ms(10); // remove
        
        event = state_low_power_sleep_handler();
        printf("event %d \r\n", event);

        time_operations_get_time_us();
        printf("time %d \r\n", time_operations_get_time_us());

        printf("TIMESERIE reference: %llu \n",temperatureSensor1_time_series.time_reference);
        print_sensor_temperatures(temperatureSensor1_queue);
        print_sensor_temperatures(temperatureSensor2_queue);

        printf("===== END =====\r\n");


#if PL_CONFIG_USE_BUTTONS
        uint32_t buttons = BTN_GetButtons();
        if (buttons!=0) {
        //Debounce_StartDebounce(buttons);
        }
#endif
    }
    return 0;
}

void heartbeat_status(void) {
    //printf("hearbeat %d \r\n", heartbeat);
    gpio_put(CYW43_WL_GPIO_LED_PIN, heartbeat);
    heartbeat = !heartbeat;
    return;
}

void print_pico_unique_id(void) {
    printf("pico unique id: %llu \n", pico_unique_board_id.id);
}

/* does not work

void save_temperature_sensor_to_queue(time_series_sensor_t *time_series, sensor_temp_t *sensor) {
    if (queue_try_add(time_series->queue, sensor)) {
        printf("QUEUE add success\r\n");
    }
    return;
}

void update_temperature_sensor(i2c_inst_t *i2c, const uint i2c_addr, queue_t *temperatureSensor_queue){
    // fixme : syntax
    sensor_temp_t temperatureSensor = {0,0, 0};
    uint16_t data_16;
    uint16_t id;

    id = tmp117_read_id(i2c, i2c_addr);

    printf("TMP117: Sensor ID %d\r\n", id);
    if(id != 0) {
        data_16 = tmp117_read_temperature(i2c, i2c_addr);
        temperatureSensor.temperature = tmp117_temperature_to_celsius(data_16);
        temperatureSensor.id += 1;
        if (queue_try_add(temperatureSensor_queue, &temperatureSensor)) {
            printf("QUEUE add success\r\n");
        }
        printf("TMP117, Temp %f\r\n", temperatureSensor.temperature);
        printf("TMP117, Data %d\r\n", data_16);
    } else {
        printf("ERROR: Could not communicate with TMP117\r\n");
    }
}
*/