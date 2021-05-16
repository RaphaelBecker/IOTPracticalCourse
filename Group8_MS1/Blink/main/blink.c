#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

//values defined by menuconfig
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BLINK_LENGTH CONFIG_BLINK_LENGTH
#define BLINK_FREQ CONFIG_BLINK_FREQ
#define LED_BUTTON_GPIO CONFIG_LED_BUTTON_GPIO
#define BUTTON_GPIO CONFIG_BUTTON_GPIO

static const char *TAG = "BLINK";

//Blink an LED attached to BLINK_GPIO with length BLINK_LENGTH and frequency BLINK_FREQ
static void vBlinkTask(void *arg)
{
	while (1)
	{

		/* Blink on (output high) */
		ESP_LOGI(TAG, "Turning on the LED");
		gpio_set_level(BLINK_GPIO, 1);
		vTaskDelay(BLINK_LENGTH / portTICK_PERIOD_MS);
		/* Blink off (output low) */
		ESP_LOGI(TAG, "Turning off the LED");
		gpio_set_level(BLINK_GPIO, 0);
		vTaskDelay(BLINK_FREQ / portTICK_PERIOD_MS);
	}
}

//Turn on an LED attached to LED_BUTTON_GPIO by pressing button at BUTTON_GPIO
static void vBlinkButtonTask(void *arg)
{
	int16_t sButtonLevel = 0;
	bool bLedOn = false;
	while(1)
	{
		sButtonLevel = gpio_get_level(BUTTON_GPIO);
		if (sButtonLevel==0)
		{
			if (!bLedOn)
			{
				ESP_LOGI("BUTTON", "Button pressed");
				bLedOn = true;
			}
			
			gpio_set_level(LED_BUTTON_GPIO, 1);
		}
		else
		{
			gpio_set_level(LED_BUTTON_GPIO, 0);
			if (bLedOn)
			{
				ESP_LOGI("BUTTON", "Button released");
				bLedOn = false;
			}
		}
		//Delay of 10ms to stop watchdog triggering
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	//esp_log_level_set("BLINK", ESP_LOG_ERROR);
	esp_log_level_set(TAG, ESP_LOG_INFO);

	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_BUTTON_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

	xTaskCreate(vBlinkTask, "blink_task", 2048, NULL, 10, NULL);
	xTaskCreate(vBlinkButtonTask, "button_task", 2048, NULL, 10, NULL);

}
