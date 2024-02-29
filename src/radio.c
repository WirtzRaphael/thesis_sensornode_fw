#include <stdio.h>

#include "McuWait.h"

#include "hardware/uart.h"
#include "pico_config.h"
#include "pico/util/queue.h"

#include "tmp117.h"
#include "sensors.h"

#define UART_RADIO_ID UART1_ID

char payload_separator_char[1] = "-";
char package_end_char[1] = ";";
char rc232_packet_end_char[2] = "LF";

typedef struct {
    char temperature[8]; // min: 4 digits + 1 point + 2 null terminator (+ 1 negative sign)
    char time_relative_to_reference[8]; // min: 5 digits (uint16)
    char id[2];
}package_sensor_temperature_t;

static void uart_wait(void) {
    sleep_ms(50); // todo : decrease
}

static void send_payload_separator(void) {
    uart_puts(UART_RADIO_ID, payload_separator_char);
    uart_wait();
}

static void send_package_end(void) {
    uart_puts(UART_RADIO_ID, package_end_char);
    uart_wait();
}

static void send_rc232_packet_end(void) {
    uart_puts(UART_RADIO_ID, rc232_packet_end_char);
    uart_wait();
}

static void create_payload_sensor_temperature(package_sensor_temperature_t *sensor_package, sensor_temp_t temperature_1_entry){
    sprintf(sensor_package->temperature, "%.2f", temperature_1_entry.temperature);
    sprintf(sensor_package->id, "%u", temperature_1_entry.id); // convert int to string
    sprintf(sensor_package->time_relative_to_reference, "%u", temperature_1_entry.time_relative_to_reference); // convert int to string
}

static void send_payload_sensor_temperature(package_sensor_temperature_t sensor_package) {
    //printf("send: %s \r\n", strTemperature);
    uart_puts(UART_RADIO_ID, sensor_package.temperature);
    uart_wait();
    send_payload_separator();
    //printf("send: %s \r\n", strId);
    uart_puts(UART_RADIO_ID, sensor_package.id);
    uart_wait();
    send_payload_separator();
    uart_wait();
    uart_puts(UART_RADIO_ID, sensor_package.time_relative_to_reference);
    uart_wait();
}

static void send_value_int(int value) {
    char value_string[4];
    sprintf(value_string, "%d", value);
    //printf("send: %s \r\n", value_string);
    uart_puts(UART_RADIO_ID, value_string);
    uart_wait();
}
/*
static void send_value_char(char value_string) {
    uart_puts(UART_RADIO_ID, value_string);
    uart_wait();
}
*/

static void reset_package_sensor_temperature(package_sensor_temperature_t *sensor_package) {
    sensor_package->temperature[0] = '\0';
    sensor_package->id[0] = '\0';
    sensor_package->time_relative_to_reference[0] = '\0';
}

static void send_time_stamp(uint64_t time_stamp) {
    char time_stamp_string[20];
    sprintf(time_stamp_string, "%lu", time_stamp);
    //printf("send: %s \r\n", time_stamp_string);
    uart_puts(UART_RADIO_ID, time_stamp_string);
    uart_wait();
}

// todo : limit message size (queue loop)
void radio_send_sensor_temperature_series(time_series_sensor_t time_series_sensor) {
    printf("send temperature sensor series\n");
    // Package start character
    uart_puts(UART_RADIO_ID, "T");
    uart_wait();
    send_payload_separator();

    send_time_stamp(time_series_sensor.time_reference);
    send_payload_separator();
    uart_puts(UART_RADIO_ID, time_series_sensor.sensor_nr);
    uart_wait();
    send_payload_separator();

    sensor_temp_t temperature_entry = {0,0, 0};
    package_sensor_temperature_t sensor_package = {0,0};
    bool first_entry = true;
    while (!queue_is_empty(time_series_sensor.queue)) {
        if (first_entry) {
            first_entry = false;
        }
        else {
            send_payload_separator(); // none on last entry
        }

        reset_temperature_entry(&temperature_entry);
        // create package
        if (queue_try_remove(time_series_sensor.queue, &temperature_entry)) {
            printf("send temperature sensor\n");
            reset_package_sensor_temperature(&sensor_package);
            create_payload_sensor_temperature(&sensor_package, temperature_entry);
            send_payload_sensor_temperature(sensor_package);
        }
    }

    send_package_end();
    send_rc232_packet_end();

    return; // remove ?
}

void radio_send_radio_module_status (void) {
    // todo
    // voltage (from rf module)
    // temperature (from rf module)
}

// todo : error info in header or payload (eg queue remove failed)
void radio_send_sensor_temperature(queue_t *temperatureSensor1_queue, queue_t *temperatureSensor2_queue) {

    sensor_temp_t temperature_1_entry = {0,0, 0}; // fixme
    if (queue_try_remove(temperatureSensor1_queue, &temperature_1_entry)) {
        /* hint : don't put at the end of the function, values change (bug)
        printf("QUEUE remove success\r\n");
        printf("QUEUE remove temperature: %f\r\n", temperature_1_entry.temperature);
        printf("QUEUE remove id: %d\r\n", temperature_1_entry.id);
        */
        printf("send temperature sensor 1\n");
        package_sensor_temperature_t sensor_package = {0,0};
        create_payload_sensor_temperature(&sensor_package, temperature_1_entry);
        send_payload_sensor_temperature(sensor_package);
        send_payload_separator();
    }

    sensor_temp_t temperature_2_entry = {0,0, 0}; // fixme
    if (queue_try_remove(temperatureSensor2_queue, &temperature_2_entry)) {
        printf("send temperature sensor 2\n");
        package_sensor_temperature_t sensor_package = {0,0};
        create_payload_sensor_temperature(&sensor_package, temperature_2_entry);
        send_payload_sensor_temperature(sensor_package);
    }
    
    send_package_end();

    return;
}

void radio_send_test_messages(void)
{
    char line1[10] = "message 1";
    printf("send: %s \r\n", line1);
    uart_puts(UART_RADIO_ID, line1);
    sleep_ms(200);
    char line2[10] = "message 2";
    printf("send: %s \r\n", line2);
    uart_puts(UART_RADIO_ID, line2);
    sleep_ms(200);
    char line3[10] = "message 3";
    printf("send: %s \r\n", line3);
    uart_puts(UART_RADIO_ID, line3);
    return;
}


// todo function serialize XY
// use sprintf
/*
void serialize(struct MyData data, char* serializedData) {
    sprintf(serializedData, "%d,%f,%s", data.intValue, data.floatValue, data.stringValue);
}
*/

// todo function deserialize XY
// use sscanf
/*
struct MyData deserialize(const char* serializedData) {
    struct MyData result;
    sscanf(serializedData, "%d,%f,%s", &result.intValue, &result.floatValue, result.stringValue);
    return result;
}
*/