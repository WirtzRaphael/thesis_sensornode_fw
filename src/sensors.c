// todo : comment block

#include "McuLog.h"
#include "McuRTOS.h"
#include "pico/time.h"

static void vSensorsTask( void * pvParameters )
{
    for( ;; )
    {
        /* Task code goes here. */
        sleep_ms(5000);
        //printf("radio killed the video star.");
    }
}

void sensors_init(void) {
  if (xTaskCreate(
      vSensorsTask,  /* pointer to the task */
      "sensors", /* task name for kernel awareness debugging */
      1000/sizeof(StackType_t), /* task stack size */
      (void*)NULL, /* optional task startup argument */
      tskIDLE_PRIORITY+2,  /* initial priority */
      (TaskHandle_t*)NULL /* optional task handle to create */
    ) != pdPASS)
  {
    for(;;){} /* error! probably out of memory */
  }
}