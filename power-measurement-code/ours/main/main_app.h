#ifndef MAINAPP_H
#define MAINAPP_H

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
uint8_t lastPublishedCount;
volatile uint8_t count;
uint8_t prev_count;
volatile uint8_t internalCount;
uint8_t triggerPinInFlag;
uint8_t triggerPinOutFlag;

long long last_ping;

#endif