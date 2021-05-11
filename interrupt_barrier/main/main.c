/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_IO_0    18
#define GPIO_OUTPUT_IO_1    19
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_IO_1     5
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

// Current number of people in the room
volatile uint8_t count;
// #### pattern definition ####:
// enter_pattern_0:
int sensor_enter_pattern_0[4] = {4, 5, 4, 5};
int sensor_enter_level_pattern_0[4] = {1, 1, 0, 0};
// enter_pattern_1:
int sensor_enter_pattern_1[4] = {4, 4, 5, 5};
int sensor_enter_level_pattern_1[4] = {1, 0, 1, 0};
// exit  pattern_0:
int sensor_exit_pattern_0[4] = {5, 4, 5, 4};
int sensor_exit_level_pattern_0[4] = {1, 1, 0, 0};
// exit pattern_1:
int sensor_exit_pattern_1[4] = {5, 5, 4, 4};
int sensor_exit_level_pattern_1[4] = {1, 0, 1, 0};

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// shifts the elements of an passed array 1 int to the left
static void shift_to_left(int *array, int size, int last_elem){
	for(int i = 0; i < 3; i++) { 
		array[i] = array[i + 1]; 
	}
	array[size -1] = last_elem;
}

static void print_array(int * array, int size) {
	for (int i = 0; i < size; i++) {
		printf("%d", array[i]);
	}
	printf("\n");
}
static bool compare_arrays(int * array1, int * array2, int size) {
	int counter = size;
	for (int i = 0; i < size; i++){
		if (array1[i] == array2[i])
			counter--;
	}
	if (counter == 0) {
		return true;
	} else {
		return false;
	}
}

static void reset_arrays(int * sensor_container, int * sensor_level_container, int size) {
	for (int i = 0; i < size; i++) {
		sensor_container[i] = 0;
		sensor_level_container[i] = 0;
	}
}

static void detect_crossing_barrier_by_pattern(int *sensor_container, int *sensor_level_container){
	// checks for enter/exit pattern 0/1  
	if (compare_arrays(sensor_container, sensor_enter_pattern_0, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_enter_level_pattern_0, sizeof(&sensor_container))) {
		printf("Enter pattern 0 recognized:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_enter_pattern_0, 4);
		print_array(sensor_level_container, 4);
		print_array(sensor_enter_level_pattern_0, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// add person to room!
		count++;
	} else if (compare_arrays(sensor_container, sensor_enter_pattern_1, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_enter_level_pattern_1, sizeof(&sensor_container))) {
		printf("Enter pattern 1 recognized:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_enter_pattern_1, 4);
		print_array(sensor_level_container, 4);
		print_array(sensor_enter_level_pattern_1, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// add person to room!
		count++;
	} else if (compare_arrays(sensor_container, sensor_exit_pattern_0, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_exit_level_pattern_0, sizeof(&sensor_container))) {
		printf("Exit pattern 0 recognized:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_exit_pattern_0, 4);
		print_array(sensor_level_container, 4);
		print_array(sensor_exit_level_pattern_0, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// substract person from room!
		count--;
	} else if (compare_arrays(sensor_container, sensor_exit_pattern_1, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_exit_level_pattern_1, sizeof(&sensor_container))) {
		printf("Exit pattern 1 recognized:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_exit_pattern_1, 4);
		print_array(sensor_level_container, 4);
		print_array(sensor_exit_level_pattern_1, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// substract person from room!
		count--;
	} 
}

static void gpio_task_detect_interrupt(void* arg)
{
	// gpio number
    uint32_t io_num;
	// gpio 4 sensor high/low status
	bool gpio_4_level_set = false;
	// gpio 5 sensor high/low status
	bool gpio_5_level_set = false;
	// array container for collecting sensor status history
	int sensor_level_container[4] = { 0 };
	int sensor_container[4] = { 0 };
	for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			// gpio4 sensor rising edge:
			if(io_num == 4 && gpio_get_level(io_num) == 1 && gpio_4_level_set == 0) {
				// debugging:
				printf("GPIO[%d] intr, val: %d, alr_setted: %d\n", io_num, gpio_get_level(io_num), gpio_4_level_set);
				gpio_4_level_set = true; // true
				shift_to_left(sensor_level_container, 4, gpio_get_level(io_num));
    			shift_to_left(sensor_container, 4, io_num);
			}
			// gpio5 sensor rising edge:
			else if(io_num == 5 && gpio_get_level(io_num) == 1 && gpio_5_level_set == 0) {
				// debugging:
				printf("GPIO[%d] intr, val: %d, alr_setted: %d\n", io_num, gpio_get_level(io_num), gpio_5_level_set);
				gpio_5_level_set = true; // true
				shift_to_left(sensor_level_container, 4, gpio_get_level(io_num));
    			shift_to_left(sensor_container, 4, io_num);
			}
			// gpio4 sensor falling edge:
			else if(io_num == 4 && gpio_get_level(io_num) == 0 && gpio_4_level_set == 1) {
				// debugging:
				printf("GPIO[%d] intr, val: %d, alr_setted: %d\n", io_num, gpio_get_level(io_num), gpio_4_level_set);
				gpio_4_level_set = false; // false
				shift_to_left(sensor_level_container, 4, gpio_get_level(io_num));
    			shift_to_left(sensor_container, 4, io_num);
			}
			// gpio4 sensor falling edge:
			else if(io_num == 5 && gpio_get_level(io_num) == 0 && gpio_5_level_set == 1) {
				// debugging:
				printf("GPIO[%d] intr, val: %d, alr_setted: %d\n", io_num, gpio_get_level(io_num), gpio_5_level_set);
				gpio_5_level_set = false; // false
				shift_to_left(sensor_level_container, 4, gpio_get_level(io_num));
    			shift_to_left(sensor_container, 4, io_num);
			}
			// check for pattern recognission:
			detect_crossing_barrier_by_pattern(sensor_container, sensor_level_container);
			// debouncing delay
			vTaskDelay(50 / portTICK_PERIOD_MS);
    	}
	}
}

void app_main(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

	// Set PIN 4 to pulldown (new)
	gpio_pulldown_en(GPIO_INPUT_IO_0);
	gpio_pulldown_en(GPIO_INPUT_IO_1);

    //change gpio intrrupt type for one pin (new)
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);
	gpio_set_intr_type(GPIO_INPUT_IO_1, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_detect_interrupt, "gpio_task_detect_interrupt", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin (new)
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
	gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
	gpio_isr_handler_remove(GPIO_INPUT_IO_1);
    //hook isr handler for specific gpio pin again (new)
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
	gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;
    while(1) {
        printf("cnt: %d, room count: %d\n", cnt++, count);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
        gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    }
}