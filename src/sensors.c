/**
 * @file sensors.c
 * @author Raphael Wirtz
 * @brief Manages sensors in the system
 * @date 2024-05-11
 *
 * @copyright Copyright (c) 2024
 *
 * todo : save sensor values format to reduce payload for transmission
 */
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

// note hw v1 : i2c0, i2c1 pins of plugs not the same
// note hw v1 : i2c1 x14 floating
#define I2Cx             i2c1
#define SENSORS_I2Cx_SCL PICO_PINS_I2C1_SCL
#define SENSORS_I2Cx_SDA PICO_PINS_I2C1_SDA
#define PRINTF_SENSORS   (0)
#define LOG_SENSORS      (0)

static error_t error;
static uint16_t id;
static uint16_t sampling_time_temparature_ms = 1000;

// todo review : extern definition in header and here
QueueHandle_t xQueue_temperature_sensor_1;
QueueHandle_t xQueue_temperature_sensor_2;
/* replaced by xQueue
static queue_t temperatureSensor1_queue;
static queue_t temperatureSensor2_queue;
*/

static temperature_measurement_t temperature_measurment_sensor1 = {
    .temperature = 0.0f, .id = 0, .timediff_to_start = 0};
static temperature_measurement_t temperature_measurment_sensor2 = {
    .temperature = 0.0f, .id = 0, .timediff_to_start = 0};

temperature_sensor_t temperatureSensor1 = {.i2c = I2Cx,
                                           .i2c_address = TMP117_1_ADDR,
                                           .sensor_nr = 1,
                                           .start_measurement_time = 0,
                                           .measurments_xQueue =
                                               &xQueue_temperature_sensor_1};
//&temperatureSensor1_queue};
temperature_sensor_t temperatureSensor2 = {.i2c = I2Cx,
                                           .i2c_address = TMP117_2_ADDR,
                                           .sensor_nr = 2,
                                           .start_measurement_time = 0,
                                           .measurments_xQueue =
                                               &xQueue_temperature_sensor_2};
//&temperatureSensor2_queue};
/**
 * @brief Sensors task
 *
 * @param pvParameters
 */
static void vSensorsTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xDelay_sensor_sampling =
      pdMS_TO_TICKS(sampling_time_temparature_ms);

  for (;;) {
    /* Task code goes here. */
    // read temperature
    sensors_read_temperature(&temperatureSensor1,
                             &temperature_measurment_sensor1);
    /*
    add_temperature_to_queue(temperatureSensor1.measurments_queue,
                             &temperature_measurment_sensor1);
    */
    add_temperature_to_xQueue(xQueue_temperature_sensor_1,
                              &temperature_measurment_sensor1);
    //
    sensors_read_temperature(&temperatureSensor2,
                             &temperature_measurment_sensor2);
    /*
    add_temperature_to_queue(temperatureSensor2.measurments_queue,
                             &temperature_measurment_sensor2);
    */
    add_temperature_to_xQueue(xQueue_temperature_sensor_2,
                              &temperature_measurment_sensor2);

    // check queue content
    /* sensors_print_temperatures_queue_peak(); */
    // sensors_print_temperatures_xQueue_latest();

    // periodic task
    vTaskDelayUntil(&xLastWakeTime, xDelay_sensor_sampling);
  }
}

/**
 * @brief Initialize the sensors task
 *
 */
void sensors_init(void) {
  /* I2C
   */
  i2c_operations_init(SENSORS_I2Cx_SDA, SENSORS_I2Cx_SCL);
  i2c_init(I2Cx, 400 * 1000); // 400 kHz

  // Queue of sensor values
  // fixme : queue length / memory usage
  const int QUEUE_LENGTH = 500;
  /*
  queue_init(temperatureSensor1.measurments_queue,
             sizeof(temperature_measurement_t), QUEUE_LENGTH);
  queue_init(temperatureSensor2.measurments_queue,
             sizeof(temperature_measurement_t), QUEUE_LENGTH);
  */
  // RTOS Queue of sensor values
  xQueue_temperature_sensor_1 =
      xQueueCreate(QUEUE_LENGTH, sizeof(temperature_measurement_t));
  xQueue_temperature_sensor_2 =
      xQueueCreate(QUEUE_LENGTH, sizeof(temperature_measurement_t));

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
 * @brief Deinitialize sensors
 *
 */
void sensors_deinit(void) { i2c_deinit(I2Cx); }

/**
 * @brief Get sampling time of sensor
 *
 * @return uint16_t
 */
uint16_t sensor_get_sampling_time(void) { return sampling_time_temparature_ms; }

/**
 * @brief Read temperature from sensor and write to sensor struct
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
#if LOG_SENSORS
  McuLog_trace("Sensor TMP117: number %d, Celsius %f\n",
               temperature_sensor->sensor_nr,
               temperature_measurement->temperature);
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
#if LOG_SENSORS
    McuLog_trace("Temperature sensor queue add success\n");
#endif
    return ERR_OK;
  } else {
    McuLog_error("Temperature sensor queue add failed\n");
    return ERR_FAILED;
  }
}

/**
 * @brief Add temperature to xQueue
 *
 * @param xQueue_temperature
 * @param temperature
 * @return error_t
 */
static error_t
add_temperature_to_xQueue(QueueHandle_t xQueue_temperature,
                          temperature_measurement_t *temperature) {
  if (xQueue_temperature == 0) {
    McuLog_error("Temperature sensor xQueue not created\n");
    return ERR_FAILED;
  }
  if (xQueueSendToBack(xQueue_temperature, temperature, 0) == pdPASS) {
#if LOG_SENSORS
    McuLog_trace("Temperature sensor xQueue add success\n");
#endif
    return ERR_OK;
  } else {
    McuLog_error("Temperature sensor xQueue add failed\n");
    return ERR_FAILED;
  }
}

/*
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
*/

/**
 * @brief Get latest temperature from xQueue
 *
 * @param xQueue_temperature
 * @param temperature
 * @return error_t
 */
error_t
sensors_temperature_xQueue_receive(QueueHandle_t xQueue_temperature,
                                   temperature_measurement_t *temperature) {
  if (xQueue_temperature == 0) {
    McuLog_error("Temperature sensor xQueue not existing\n");
    return ERR_FAILED;
  }
  if (xQueueReceive(xQueue_temperature, temperature, 0) == pdPASS) {
    McuLog_trace("Temperature sensor xQueue receive success\n");
    return ERR_OK;
  } else {
    McuLog_error("Temperature sensor xQueue receive failed\n");
    return ERR_FAILED;
  }
}

/**
 * @brief Print all latest temperature measurments
 *
 */
void sensors_print_temperature_xQueue_latest_all(void) {
  sensors_print_temperature_xQueue_latest(xQueue_temperature_sensor_1);
  sensors_print_temperature_xQueue_latest(xQueue_temperature_sensor_2);
}

/**
 * @brief Print latest temperature measurment from xQueue
 *
 * @param xQueue_temperature
 */
void sensors_print_temperature_xQueue_latest(QueueHandle_t xQueue_temperature) {
  temperature_measurement_t temperature_measurement;
  if (xQueue_temperature == 0) {
    McuLog_error("Temperature sensor xQueue not existing\n");
    return;
  }
  if (xQueuePeek(xQueue_temperature, &temperature_measurement, 0) == pdPASS) {
    McuLog_trace("temperature: %f\n", temperature_measurement.temperature);
    printf("temperature: %f\n", temperature_measurement.temperature);
  } else {
    McuLog_error("Temperature sensor xQueue receive failed\n");
  }
}

/*
void sensors_print_temperatures_queue_peak(void) {
  print_temperature_queue_peak(temperatureSensor1.measurments_queue);
  print_temperature_queue_peak(temperatureSensor2.measurments_queue);
}

static void print_temperature_queue_peak(queue_t *temperature_sensor_queue) {
  float temperature = 0.0;
  error =
      sensors_get_latest_temperature(temperature_sensor_queue, &temperature);
  if (error == ERR_OK) {
    McuLog_trace("temperature (xQueue): %f\n", temperature);
    printf("temperature (xQueue): %f\n", temperature);
  } else {
  }
}
*/
