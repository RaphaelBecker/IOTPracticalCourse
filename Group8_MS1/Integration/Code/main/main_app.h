#ifndef MAINAPP_H
#define MAINAPP_H
//Define to exclude data recording - dont wanna record data from my personal testing board
#define TESTING

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

// Room count:
volatile uint8_t count;
volatile uint8_t internalCount;
uint8_t triggerPinInFlag;
uint8_t triggerPinOutFlag;
uint8_t prev_triggerPinOutFlag;
uint8_t prev_triggerPinInFlag;

#endif