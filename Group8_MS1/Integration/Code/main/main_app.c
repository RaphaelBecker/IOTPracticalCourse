
#include "main_app.h"

//Custom Headers
#include "display.h"
#include "wifiController.h"
#include "sntpController.h"
#include "mqttInterface.h"
#include "roomMonitoring.h"
#include "commands.h"
#include "http_client.h"

static const char *TAG = "ROOM";

static void restartDevice()
{
	struct tm timeinfo = {0};
	time_t now = 0;
	while (1)
	{
		//call once every hour
		vTaskDelay(3600000 / portTICK_PERIOD_MS);

		//check if its 3am
		time(&now);
		localtime_r(&now, &timeinfo);
		if (timeinfo.tm_hour == 3)
		{
			//Set count to 0 if not 0
			if (count != 0)
			{
				count = 0;
				mqttPublishCount();
				vTaskDelay(5000 / portTICK_PERIOD_MS);
			}
			//Restart esp32
			esp_restart();
		}
	}
}


void app_main(void)
{
	count = 0;
	internalCount = 0;
	ESP_LOGI(TAG, "Boot sequence finished, starting app_main");

	ESP_LOGI(TAG, "[APP] Startup..");
	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

	//Set log level
	esp_log_level_set(TAG, ESP_LOG_INFO);

	esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
	esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
	esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
	esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	//Initialize Display
	initDisplay();

	//Connect to the Wifi Network
	connectWifi();

	//Initialize SNTP
	initializeSntp();

	//Call SNTP to sync time with server
	obtainTime();

	//Check iot platform api for latest submitted count to initialize our counter
	checkAPIforLatestCount();

	//starts mqtt handler
	mqtt_app_start();

	// configures room monitoring
	configureRoomMonitoring();

	//Start Tasks:

	//update oled display and show Room status
	xTaskCreate(showRoomState, "DisplayRoomState", 2048, NULL, 10, NULL);

	//updates timestamp from SNTP (Simple Network Time Protocol)
	xTaskCreate(vUpdateTimeStamp, "TimeStamp", 1024, NULL, 5, NULL);

	//publishes a restart event
	xTaskCreate(mqttPublishRestart, "PublishRestart", 2048, NULL, 5, NULL);

	//publishes the room count
	xTaskCreate(mqttPublishCountTask, "PublishCountPeriod", 4096, NULL, 10, NULL);

	//restarts the device at 3am every day
	xTaskCreate(restartDevice, "RestartAtNight", 1024, NULL, 20, NULL);
}
