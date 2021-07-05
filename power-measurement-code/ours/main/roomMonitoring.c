#include "roomMonitoring.h"
#include "commands.h"
#include "main_app.h"
#include "mqttInterface.h"
#include "counter.h"
#include <sys/time.h>

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

volatile uint8_t sensorBuffer[8];
volatile uint16_t timestampbuffer[8];
static const char *TAG = "ROOM_MONITOR";
long startManipulation;
long stopManipulation;
bool manipulationFlag = 0;

//triggerPinIn = 0;
//triggerPinOut = 2;

void publishCountOnChange()
{
    while (1)
    {
        if (count != lastPublishedCount)
        {
            lastPublishedCount = count;
            mqttPublishCount();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void publishCountEverySecond()
{
    while (1)
    {
        mqttPublishCount();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void monitorTriggerPinFlags()
{
    for (;;)
    {
        //Run task every 10 ms
        vTaskDelay(10 / portTICK_RATE_MS);

        //Save interrupts to array
        if (triggerPinInFlag > 0)
        {
            xTaskCreate(insertSignalPinToArrayBuffer, "insertPin", 2048, (void *)triggerPinIn, 3, NULL);
            triggerPinInFlag = 0;
        }
        if (triggerPinOutFlag > 0)
        {
            xTaskCreate(insertSignalPinToArrayBuffer, "insertPin", 2048, (void *)triggerPinOut, 3, NULL);
            triggerPinOutFlag = 0;
        }
    }
}

//Interrupt Handlers
static void IRAM_ATTR triggerPinInHandler(void *arg)
{
    triggerPinInFlag++;
}

static void IRAM_ATTR triggerPinOutHandler(void *arg)
{
    triggerPinOutFlag++;
}

//Setup
void configureRoomMonitoring()
{
    //initialize variables for interrupts
    triggerPinInFlag = 0;
    triggerPinOutFlag = 0;

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    //Configure GPIO pins
    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(triggerPinIn, GPIO_MODE_INPUT);
    gpio_set_direction(triggerPinOut, GPIO_MODE_INPUT);

    gpio_set_intr_type(triggerPinIn, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(triggerPinOut, GPIO_INTR_ANYEDGE);

    //Set interrupt handlers for triggerPinIn and triggerPinOut
    gpio_isr_handler_add(triggerPinIn, triggerPinInHandler, (void *)triggerPinIn);
    gpio_isr_handler_add(triggerPinOut, triggerPinOutHandler, (void *)triggerPinOut);

    //creates task to monitor the triggerPinInFlag and triggerPinOutFlag, which triggers the counter algorithm by increment
    xTaskCreate(monitorTriggerPinFlags, "monitorTriggerPinFlags", 4096, NULL, 15, NULL);
}
