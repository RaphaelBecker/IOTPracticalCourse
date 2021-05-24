#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "roomMonitoring.h"

// use for debugging:
//printf("GPIO[%d] interupt, val: %d \n", triggerPinIn, gpio_get_level(triggerPinIn));

static const char *TAG = "COMMANDS";

void leaveRoom()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Leave: -1");
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(500 / portTICK_RATE_MS);
}

void enterRoom()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Enter: +1");
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(500 / portTICK_RATE_MS);
}

void ping()
{
	ESP_LOGI(TAG, "Command: ping");
	gpio_set_level(CONFIG_LED_GPIO, 1);
	vTaskDelay(2500 / portTICK_PERIOD_MS);
	gpio_set_level(CONFIG_LED_GPIO, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	//expected outcome no change
}

/******************************************
* Corner cases that only invlove 1 person *
*******************************************/

void halfwayEnter()
{
	/**
 * someone goes to the middle of the doorway, and then turns around
 * expected count result: no change
 */
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Half Enter: NO change");
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
}

/**
 * Corner case from Group 4: Almost Enter (big person). 
 * When the student decides to turn around the student has already
 * unblocked the outer barrier but not the inner one.
 * expected outcome: no change
 */
void breaksOuterAndInnerButReturnsG4()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: breakOuterAndInnerButReturnsG4: NO change");
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(200 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(200 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(200 / portTICK_PERIOD_MS);

	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(200 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(200 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(200 / portTICK_PERIOD_MS);
}

/** 
 * Corner case from Group9: Almost Enter (slim person).
 * a person enters the room (breaking the first and then the second light barrier),
 * turns around (while the second light barrier is still broken),
 * and leaves the rooms (breaking the first light barrier again quickly after).
 * expected outcome: no change
 */
void personTurnedG9()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Person entered the room and turned around: NO change");
	// person entering
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	// person turning around
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(300 / portTICK_PERIOD_MS); // turning takes time
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	// person left the room again
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
}

/** 
 * someone almost enters the room, but takes one step back (Pin goes low before PinOut)
 * before finally entering.
 * expected outcome: +1
 */
void unsureEnter()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Unsure Enter: +1");
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
}

/**************************************
 * Corner cases where time is critical *
 **************************************/
/**
 * Someone is trying to manipulate the count by waving their arm through the barrier towards the inside
 * Sequence is not possible if a person enters
 * expected outcome: no change
 */
void manipulationEnter()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Manipulation Enter. NO change ");
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(15 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(15 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(15 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
}

/**********************************************
 * Corner cases that invlove multiple people *
 **********************************************/

/** 
 * Corner case from Group11:
 * Alice peeks into the room, shortly afterwards Bob leaves the room
 * expected count result: -1
 */
void peeketoandLeaveG11()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Peek into and leave: -1");
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);

	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(500 / portTICK_PERIOD_MS);
}

/** 
 * successive entering
 * Alice enters the room while Bob also enters
 * expected outcome: +2
 */
void successiveEnter()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Command: Successive Enter: +2");
	// first person entering
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	// first person almost inside
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	// second person entering
	gpio_set_level(triggerPinOut, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	// first person inside
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	// second person entering
	gpio_set_level(triggerPinOut, 0);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	gpio_set_level(triggerPinIn, 0);
	vTaskDelay(500 / portTICK_PERIOD_MS);
}