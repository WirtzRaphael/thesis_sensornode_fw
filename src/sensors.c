// todo : comment block
// todo : refactor
// todo : clen includes
#include "sensors.h"
#include "McuLib.h"
#include "McuLog.h"
#include "McuRTOS.h"
#include "hardware/i2c.h"
#include "pico/util/queue.h"

#include "i2c_operations.h"
#include "pico_config.h"
#include "tmp117.h"
#include <errno.h>
#include <stdint.h>

#define PRINTF_SENSORS (1)

error_t error;
queue_t temperatureSensor1_queue;
queue_t temperatureSensor2_queue;

uint16_t sampling_time_temparature_ms = 2000;

uint16_t id;
uint16_t measurement_value;
uint16_t measurement_value_celsius;

#define I2Cx i2c0

// note hw v1 : i2c0, i2c1 pins of plugs not the same
// note hw v1 : i2c1 x14 floating
temperature_sensor_t temperatureSensor1 = {.i2c = I2Cx,
                                           .i2c_address = TMP117_1_ADDR,
                                           .sensor_nr = 1,
                                           .start_measurement_time = 0,
                                           .measurments =
                                               &temperatureSensor1_queue};
temperature_sensor_t temperatureSensor2 = {.i2c = I2Cx,
                                           .i2c_address = TMP117_2_ADDR,
                                           .sensor_nr = 2,
                                           .start_measurement_time = 0,
                                           .measurments =
                                               &temperatureSensor2_queue};

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
    // read temperature
    sensors_read_temperature(&temperatureSensor1,
                             &temperature_measurment_sensor1);
    add_temperature_to_queue(&temperatureSensor1_queue,
                             &temperature_measurment_sensor1);
    sensors_read_temperature(&temperatureSensor2,
                             &temperature_measurment_sensor2);
    add_temperature_to_queue(&temperatureSensor2_queue,
                             &temperature_measurment_sensor2);

    // check queue content
    sensor_print_latest_temperatures(&temperatureSensor1_queue);
    sensor_print_latest_temperatures(&temperatureSensor2_queue);

    // periodic task
    vTaskDelayUntil(&xLastWakeTime,
                    pdMS_TO_TICKS(sampling_time_temparature_ms));
  }
}

/**
 * @brief Initialize sensors
 *
 */
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

/**
 * @brief Get sampling time of sensor
 *
 * @return uint16_t
 */
uint16_t sensor_get_sampling_time(void) { return sampling_time_temparature_ms; }

/**
 * @brief Read temperature from sensor
 *
 * @param temperature_sensor
 * @param temperature_measurement
 * @return error_t
 */
error_t
sensors_read_temperature(temperature_sensor_t *temperature_sensor,
                         temperature_measurement_t *temperature_measurement) {
  // Check i2c connection
  id = tmp117_read_id(temperature_sensor->i2c, temperature_sensor->i2c_address);
  if (id == 0) {
    McuLog_error("TMP117: Could not communicate with sensor number %d\n",
                 temperature_sensor->sensor_nr);
    return ERR_FAILED;
  }
  temperature_measurement_t temp_measurement;

  // Read temperature
  temp_measurement.temperature = tmp117_read_temperature_in_celsius(
      temperature_sensor->i2c, temperature_sensor->i2c_address);
  temp_measurement.id += 1;

  // Write measurement in sensor struct
  temperature_measurement->temperature = tmp117_read_temperature_in_celsius(
      temperature_sensor->i2c, temperature_sensor->i2c_address);
  temperature_measurement->id += 1;
  // todo : time function
  temperature_measurement->timediff_to_start = 1;

  // Output
  McuLog_trace("Sensor TMP117: number %d, Celsius %f\n",
               temperature_sensor->sensor_nr,
               temperature_measurement->temperature);
#if PRINTF_SENSORS
  printf("Sensor TMP117: (number %d, Celsius %f\n",
         temperature_sensor->sensor_nr, temperature_measurement->temperature);
#endif
  return ERR_OK;
}

/**
 * @brief Add temperature to queue
 *
 * @param temperature_sensor_queue
 * @param temperature
 * @return error_t
 */
static error_t
add_temperature_to_queue(queue_t *temperature_sensor_queue,
                         temperature_measurement_t *temperature) {
  if (queue_try_add(temperature_sensor_queue, temperature)) {
    return ERR_OK;
  } else {
    McuLog_error("Temperature sensor queue add failed\n");
    return ERR_FAILED;
  }
}

/**
 * @brief Get latest temperature from queue
 *
 * @param temperature_sensor_queue
 * @param temperature
 * @return error_t
 */
error_t sensors_get_latest_temperature(queue_t *temperature_sensor_queue,
                                       float *temperature) {
  temperature_measurement_t temperature_measurement;
  if (queue_try_peek(temperature_sensor_queue, &temperature_measurement)) {
    *temperature = temperature_measurement.temperature;
    return ERR_OK;
  } else {
    McuLog_error("Temperature sensor queue peek failed\n");
    return ERR_FAILED;
  }
}

void sensor_print_latest_temperatures(queue_t *temperature_sensor_queue) {
  float temperature = 0.0;
  error =
      sensors_get_latest_temperature(temperature_sensor_queue, &temperature);
  if (error == ERR_OK) {
    printf("temperature: %f\n", temperature);
  } else {
  }
}