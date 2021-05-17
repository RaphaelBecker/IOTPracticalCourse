
#include "main_app.h"

//Custom Headers
#include "display.h"
#include "wifiController.h"
#include "sntpController.h"
#include "mqttInterface.h"


static const char *TAG = "ROOM";

#define LED_GPIO CONFIG_LED_GPIO



int ledOn = 0;

void toggleLED()
{
	if (ledOn == 0)
	{
		ESP_LOGI(TAG, "Turning on the LED");
		displayText("LED On");
		gpio_set_level(LED_GPIO, 1);
		ledOn = 1;
	}
	else
	{
		ESP_LOGI(TAG, "Turning off the LED");
		gpio_set_level(LED_GPIO, 0);
		displayText("LED Off");
		ledOn = 0;
	}
	//textDemo();
}



void app_main(void)
{

	initDisplay();
	//textDemo();

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

	xTaskCreate(vDisplayTask,"Display",2048, NULL, 10, NULL);
	xTaskCreate(vUpdateTimeStamp,"TimeStamp",1024, NULL, 5, NULL);


	//mqtt_app_start();

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
