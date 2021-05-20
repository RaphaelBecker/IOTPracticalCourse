#include "roomMonitoring.h"
#include "commands.h"
#include "main_app.h"
//#include "counter.h"

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

//triggerPinIn = 0;
//triggerPinOut = 2;

void monitorTriggerPinFlags(){
    prev_triggerPinInFlag = triggerPinInFlag;
    prev_triggerPinOutFlag = triggerPinOutFlag;
    for(;;)
    {
        vTaskDelay(20 / portTICK_RATE_MS);
        if(triggerPinInFlag == prev_triggerPinInFlag + 1)
        {
            prev_triggerPinInFlag = triggerPinInFlag;
            //debugging:
            printf("GPIO[%d] interupt, val: %d \n", triggerPinIn, gpio_get_level(triggerPinIn));
        }
        if(triggerPinOutFlag == prev_triggerPinOutFlag + 1)
        {
            prev_triggerPinOutFlag = triggerPinOutFlag;
            //debugging:
            printf("GPIO[%d] interupt, val: %d \n", triggerPinOut, gpio_get_level(triggerPinOut));
        }
    }
}

static void IRAM_ATTR triggerPinInHandler(void* arg)
{
    triggerPinInFlag++;
}

static void IRAM_ATTR triggerPinOutHandler(void* arg)
{
    triggerPinOutFlag++;
}

void executeCountingAlgoTests()
{
    leaveRoom();
    enterRoom();

    //Corner cases
    halfwayEnter();
    breaksOuterAndInnerButReturnsG4();
    personTurnedG9();
    unsureEnter();
    manipulationEnter();
    peeketoandLeaveG11();
    successiveEnter();
}

void configureRoomMonitoring()
{
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(triggerPinIn, GPIO_MODE_INPUT_OUTPUT);

    gpio_set_direction(triggerPinOut, GPIO_MODE_INPUT_OUTPUT);
    
    gpio_set_intr_type(triggerPinIn, GPIO_INTR_ANYEDGE);

    gpio_set_intr_type(triggerPinOut, GPIO_INTR_ANYEDGE);

    //hook triggerPinInFunction for triggerPinIn gpio pin
    gpio_isr_handler_add(triggerPinIn, triggerPinInHandler,  (void*) triggerPinIn);
    //hook triggerPinOutFunction for triggerPinOut gpio pin
    gpio_isr_handler_add(triggerPinOut, triggerPinOutHandler, (void*) triggerPinOut);

    // creates task to monitor the triggerPinInFlag and triggerPinOutFlag, which triggers the counter algorithm by increment
    //xTaskCreate(monitorTriggerPinFlags, "monitorTriggerPinFlags", 4096, NULL, 15, NULL);
	
    //executes Tests to evaluate the room counter algorithm in counter.c
    //executeCountingAlgoTests();
    
    //ued for debugging:
    ESP_LOGI(TAG, "prev_triggerPinInFlag: %d", prev_triggerPinInFlag);
    ESP_LOGI(TAG, "prev_triggerPinOutFlag: %d", prev_triggerPinOutFlag);
    ESP_LOGI(TAG, "count: %d", count);

}
