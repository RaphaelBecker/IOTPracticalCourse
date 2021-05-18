#include "roomMonitoring.h"
#include "main_app.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

//triggerPinIn = 0;
//triggerPinOut = 2;

static void IRAM_ATTR test(void* arg)
{
    count++;
}

void configureRoomMonitoring()
{
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(triggerPinIn, GPIO_MODE_INPUT_OUTPUT);
    
    gpio_set_intr_type(triggerPinIn, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(triggerPinIn, test, NULL);
}