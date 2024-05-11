// todo : comment block
// todo : refactor
// todo : clen includes
#include "sensors.h"
#include "McuLib.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "pico/util/queue.h"

#include "i2c_operations.h"
#include "pico_config.h"
#include "tmp117.h"
#include <errno.h>
#include <stdint.h>

queue_t temperatureSensor1_queue;
queue_t temperatureSensor2_queue;

uint16_t sampling_time_temparature_ms = 2000;

uint16_t id;
uint16_t measurement_value;
uint16_t measurement_value_celsius;

// note hw v1 : i2c0, i2c1 pins of plugs not the same
// note hw v1 : i2c1 x14 floating
i2c_inst_t *I2Cx = i2c0;

// todo add I2C
temperature_sensor_t temperatureSensor1 = {.i2c_address = TMP117_1_ADDR,
                                           .sensor_nr = 1,
                                           .start_measurement_time = 0,
                                           .measurments =
                                               &temperatureSensor1_queue};
temperature_sensor_t temperatureSensor2 = {.i2c_address = TMP117_2_ADDR,
                                           .sensor_nr = 2,
                                           .start_measurement_time = 0,
                                           .measurments =
                                               &temperatureSensor2_queue};

// todo : de-, activate sensor
// Time series of sensor values
// todo : change format to reduce payload for transmission
temperature_measurement_t temperature_measurment_sensor1 = {
    .temperature = 0.0f, .id = 0, .timediff_to_start = 0};

temperature_measurement_t temperature_measurment_sensor2 = {
    .temperature = 0.0f, .id = 0, .timediff_to_start = 0};

static void vSensorsTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    /* Task code goes here. */
    // todo : interval time sensor read
    // read temperature
    sensors_read_temperature(I2Cx, &temperatureSensor1,
                             &temperature_measurment_sensor1);
    sensors_read_temperature(I2Cx, &temperatureSensor2,
                             &temperature_measurment_sensor2);

    // print temperature
    printf("sensors task\n");

    // wait
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(sampling_time_temparature_ms));

    // float temperature1 = get_latest_temperature(temperatureSensor1_queue);
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
  queue_init(&temperatureSensor1_queue, sizeof(temperature_measurement_t),
             QUEUE_LENGTH);
  queue_init(&temperatureSensor2_queue, sizeof(temperature_measurement_t),
             QUEUE_LENGTH);

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

uint16_t sensor_get_sampling_time(void) { return sampling_time_temparature_ms; }

error_t sensors_read_temperature(i2c_inst_t *i2c,
                         temperature_sensor_t *temperature_sensor,
                         temperature_measurement_t *temperature_measurement) {
  // id = tmp117_read_id(I2Cx, temperature_sensor->i2c_address);
  id = tmp117_read_id(I2Cx, temperature_sensor->i2c_address);
  if (id == 0) {
    printf("TMP117: Could not communicate with sensor number %d\n",
           temperature_sensor->sensor_nr);
    return ERR_FAILED;
  }
  printf("TMP117: Sensor number %d\n", temperature_sensor->sensor_nr);
  measurement_value =
      tmp117_read_temperature(I2Cx, temperature_sensor->i2c_address);
  temperature_measurement->temperature =
      tmp117_temperature_to_celsius(measurement_value);
  temperature_measurement->id += 1;
  // todo : time function
  temperature_measurement->timediff_to_start = 1;
  printf("TMP117: Temp %f\n", temperature_measurement->temperature);
  // todo : add measurements to queue

  return ERR_OK;
}

// todo : return error code, value as pointer
// todo : static
float get_latest_temperature(queue_t temperature_sensor_queue) {
  //  sensor_temp_t temperature_entry;
  //  if (queue_try_peek(&temperature_sensor_queue, &temperature_entry)) {
  //    return temperature_entry.temperature;
  //  } else {
  //    return 0.0f;
  //  }
  return 0.0f;
}

void print_sensor_temperatures(queue_t temperature_sensor_queue) {
  //  sensor_temp_t temperature_entry;
  //  if (queue_try_peek(&temperature_sensor_queue, &temperature_entry)) {
  //    printf("QUEUE peek success\r\n");
  //    printf("QUEUE peek temperature: %f\r\n", temperature_entry.temperature);
  //    printf("QUEUE peek id: %d\r\n", temperature_entry.id);
  //    printf("QUEUE peek time relative: %d\r\n",
  //           temperature_entry.time_relative_to_reference);
  //  }

  /*
  if (queue_try_remove(&temperatureSensor1_queue, &temperature_entry)) {
      printf("QUEUE remove success\r\n");
      printf("QUEUE remove temperature: %f\r\n", temperature_entry.temperature);
      printf("QUEUE remove id: %d\r\n", temperature_entry.id);
  }
  */
}