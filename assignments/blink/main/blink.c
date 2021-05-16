#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

// #define BLINK_GPIO CONFIG_BLINK_GPIO
#define BLINK_GPIO 19

static const char *TAG = "BLINK";

void app_main(void){
	//esp_log_level_set("BLINK", ESP_LOG_ERROR);       
	esp_log_level_set("BLINK", ESP_LOG_INFO);       
	
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	while(1) {
        /* Blink on (output high) */
		ESP_LOGI(TAG, "Turning on the LED");
		gpio_set_level(BLINK_GPIO, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* Blink off (output low) */
		ESP_LOGI(TAG, "Turning off the LED");
		gpio_set_level(BLINK_GPIO, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

