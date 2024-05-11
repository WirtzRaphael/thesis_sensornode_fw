// todo : comment block
// todo : refactor
#include "sensors.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "pico/util/queue.h"

#include "i2c_operations.h"
#include "pico_config.h"
#include "tmp117.h"

queue_t temperatureSensor1_queue;
queue_t temperatureSensor2_queue;

uint16_t data1_16, data2_16;
uint16_t id1, id2;

// fix : i2c1 X14 floating
// note hw v1 : pins tmp117 and plug i2c0, i2c1 not same
i2c_inst_t *I2Cx = i2c0;

sensor_temp_t temperatureSensor1 = {0, 0, 0};
sensor_temp_t temperatureSensor2 = {0, 0, 0};
// todo : de-, activate sensor
// todo : sensor 2
// sensor_temp_t temperatureSensor2 = {0,0, 0};

// Time series of sensor values
// todo : change format to reduce payload for transmission
time_series_sensor_t temperatureSensor1_time_series = {
    .sensor_nr = '1', .time_reference = 0, .queue = &temperatureSensor1_queue};

time_series_sensor_t temperatureSensor2_time_series = {
    .sensor_nr = '2', .time_reference = 0, .queue = &temperatureSensor2_queue};

static void vSensorsTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    /* Task code goes here. */
    // todo : interval time sensor read
    // read temperature
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));

    // todo : separate functions
    sensors_read_temperature(I2Cx);

    // print temperature
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));

    printf("sensors task\n");
    float temperature1 = get_latest_temperature(temperatureSensor1_queue);
    // printf("radio killed the video star.");
  }
}

void sensors_init(void) {
  /* I2C
   */
  i2c_operations_init(PICO_PINS_I2C0_SDA, PICO_PINS_I2C0_SCL);
  // Initialize I2C0 port at 400 kHz
  // i2c_inst_t *i2c_0 = i2c0;
  i2c_init(I2Cx, 400 * 1000);

  // Queue of sensor values
  const int QUEUE_LENGTH = 128;
  queue_init(&temperatureSensor1_queue, sizeof(sensor_temp_t), QUEUE_LENGTH);
  // temperatureSensor1_time_series.time_reference =
  // todo : time function
  // time_operations_get_time_us();
  temperatureSensor1_time_series.time_reference = 10;

  if (xTaskCreate(vSensorsTask, /* pointer to the task */
                  "sensors",    /* task name for kernel awareness debugging */
                  1000 / sizeof(StackType_t), /* task stack size */
                  (void *)NULL,         /* optional task startup argument */
                  tskIDLE_PRIORITY + 2, /* initial priority */
                  (TaskHandle_t *)NULL  /* optional task handle to create */
                  ) != pdPASS) {
    for (;;) {
    } /* error! probably out of memory */
  }
}

// todo : one function
void sensors_read_temperature(i2c_inst_t *i2c) {
  /* TMP117 1
   */
  // Read device ID to make sure that we can communicate with the ADXL343
  sleep_ms(100); // fixme : magic delay (no connection fix)
  // fixme : tmp117 no communication
  /* TMP117 1 */
  id1 = tmp117_read_id(i2c, TMP117_2_ADDR);
  printf("TMP117 1: Sensor ID %d\r\n", id1);
  if (id1 != 0) {
    data1_16 = tmp117_read_temperature(i2c, TMP117_2_ADDR);
    temperatureSensor1.temperature = tmp117_temperature_to_celsius(data1_16);
    temperatureSensor1.id += 1;
    // todo : time function
    temperatureSensor1.time_relative_to_reference = 1;
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
    // todo : time function
    temperatureSensor2.time_relative_to_reference = 1;
    if (queue_try_add(&temperatureSensor2_queue, &temperatureSensor2)) {
      printf("QUEUE add success\r\n");
    }
    printf("TMP117 2, Temp %f\r\n", temperatureSensor2.temperature);
    printf("TMP117 2, Data %d\r\n", data2_16);
  } else {
    printf("ERROR: Could not communicate with TMP117 2\r\n");
  }
}

// todo : return error code, value as pointer
// todo : static
float get_latest_temperature(queue_t temperature_sensor_queue) {
  sensor_temp_t temperature_entry;
  if (queue_try_peek(&temperature_sensor_queue, &temperature_entry)) {
    return temperature_entry.temperature;
  } else {
    return 0.0f;
  }
}

void print_sensor_temperatures(queue_t temperature_sensor_queue) {
  sensor_temp_t temperature_entry;
  if (queue_try_peek(&temperature_sensor_queue, &temperature_entry)) {
    printf("QUEUE peek success\r\n");
    printf("QUEUE peek temperature: %f\r\n", temperature_entry.temperature);
    printf("QUEUE peek id: %d\r\n", temperature_entry.id);
    printf("QUEUE peek time relative: %d\r\n",
           temperature_entry.time_relative_to_reference);
  }

  /*
  if (queue_try_remove(&temperatureSensor1_queue, &temperature_entry)) {
      printf("QUEUE remove success\r\n");
      printf("QUEUE remove temperature: %f\r\n", temperature_entry.temperature);
      printf("QUEUE remove id: %d\r\n", temperature_entry.id);
  }
  */
}