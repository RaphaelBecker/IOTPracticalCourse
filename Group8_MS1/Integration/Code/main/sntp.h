#ifndef SNTPCONTROLLER_H
#define SNTPCONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_sntp.h"
#include "esp_event.h"
#include "esp_log.h"

time_t now = 0;
struct tm timeinfo = {0};

void time_sync_notification_cb(struct timeval *tv);
void obtainTime();
void initializeSntp();
#endif