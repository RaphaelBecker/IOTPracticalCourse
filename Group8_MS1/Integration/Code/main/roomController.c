#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"


//Custom Headers
#include "display.h"
#include "wifiController.h"
#include "sntpController.h"


static const char *TAG = "ROOM";

const int LEDPIN = 32;
const int PushButton = 5;
const int gmtOffset_sec = 7200;



int ledOn = 0;

void toggleLED()
{
	if (ledOn == 0)
	{
		ESP_LOGI(TAG, "Turning on the LED");
		displayText("LED On");
		gpio_set_level(LEDPIN, 1);
		ledOn = 1;
	}
	else
	{
		ESP_LOGI(TAG, "Turning off the LED");
		gpio_set_level(LEDPIN, 0);
		displayText("LED Off");
		ledOn = 0;
	}
	//textDemo();
}



void app_main(void)
{

	initDisplay();
	textDemo();

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

	connectWifi();

	//esp_log_level_set("BLINK", ESP_LOG_ERROR);
	esp_log_level_set(TAG, ESP_LOG_INFO);

	//pinMode(LEDPIN, OUTPUT);
	//pinMode(PushButton, INPUT);


	//char strftime_buf[64];
	obtainTime();

	//ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

	while (1)
	{
		//strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo);
		//displayTextTime(strftime_buf, 5);
		vTaskDelay(10/portTICK_PERIOD_MS);
#ifdef LED
		/* Blink on (output high) */
		int Push_button_state = gpio_get_level(PushButton);
		if (Push_button_state == 0)
		{
			toggleLED();
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
#endif
		/*
		
		gpio_set_level(BLINK_GPIO, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
        // Blink off (output low) 
		
		gpio_set_level(BLINK_GPIO, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		*/
	}
}
