
#include "main_app.h"

//Custom Headers
#include "display.h"
#include "wifiController.h"
#include "sntpController.h"
#include "mqttInterface.h"
#include "roomMonitoring.h"
#include "commands.h"

static const char *TAG = "ROOM";

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

	initializeSntp();

	//Connect to the Wifi Network
	connectWifi();

	//Call SNTP to sync time with server
	obtainTime();

	mqtt_app_start();

	configureRoomMonitoring();

	//Start Tasks
	xTaskCreate(showRoomState, "DisplayRoomState", 2048, NULL, 10, NULL);
	xTaskCreate(vUpdateTimeStamp, "TimeStamp", 1024, NULL, 5, NULL);
	xTaskCreate(mqttPublishRestart, "PublishRestart", 2048, NULL, 5, NULL);
	xTaskCreate(mqttPublishCountTask, "PublishCountPeriod", 2048, NULL, 10, NULL);
}
