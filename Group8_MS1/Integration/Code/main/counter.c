#include "counter.h"
#include "roomMonitoring.h"

//triggerPinIn = 0;
//triggerPinOut = 2;

// #### pattern definition ####:
// enter_pattern_0:
uint8_t sensor_enter_pattern_0[4] = {2, 0, 2, 0};
uint8_t sensor_enter_level_pattern_0[4] = {1, 1, 0, 0};
// enter_pattern_1:
uint8_t sensor_enter_pattern_1[4] = {2, 2, 0, 0};
uint8_t sensor_enter_level_pattern_1[4] = {1, 0, 1, 0};
// exit  pattern_0:
uint8_t sensor_exit_pattern_0[4] = {0, 2, 0, 2};
uint8_t sensor_exit_level_pattern_0[4] = {1, 1, 0, 0};
// exit pattern_1:
uint8_t sensor_exit_pattern_1[4] = {0, 0, 2, 2};
uint8_t sensor_exit_level_pattern_1[4] = {1, 0, 1, 0};

//container for sensor signals
uint8_t sensor_level_container[4] = { 0 };
uint8_t sensor_container[4] = { 0 };

void test12()
{
    count++;
}

// shifts the elements of an passed array 1 int to the left
void shift_to_left(uint8_t *array, uint8_t size, uint8_t last_elem){
	for(int i = 0; i < 3; i++) { 
		array[i] = array[i + 1]; 
	}
	array[size - 1] = last_elem;
}

// prints passed array
void print_array(uint8_t * array, uint8_t size) {
	for (int i = 0; i < size; i++) {
		printf("%d", array[i]);
	}
	printf("\n");
}

// return true if passed arrays are equal
bool compare_arrays(uint8_t * array1, uint8_t * array2, uint8_t size) {
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

// sets all items in array to 0
void reset_arrays(uint8_t * sensor_container, uint8_t * sensor_level_container, uint8_t size) {
	for (int i = 0; i < size; i++) {
		sensor_container[i] = 0;
		sensor_level_container[i] = 0;
	}
}

void detect_crossing_barrier_by_pattern(){
	// checks for enter/exit pattern 0/1  
	if (compare_arrays(sensor_container, sensor_enter_pattern_0, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_enter_level_pattern_0, sizeof(&sensor_container))) {
		printf("Enter pattern 0 detected:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_level_container, 4);
		printf("Pattern:\n");
        print_array(sensor_enter_pattern_0, 4);
        print_array(sensor_enter_level_pattern_0, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// add person to room:
		count++;
        printf("count: %d\n", count);
	} else if (compare_arrays(sensor_container, sensor_enter_pattern_1, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_enter_level_pattern_1, sizeof(&sensor_container))) {
		printf("Enter pattern 1 detected:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_level_container, 4);
        printf("Pattern:\n");
        print_array(sensor_enter_pattern_1, 4);
		print_array(sensor_enter_level_pattern_1, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// add person to room:
		count++;
        printf("count: %d\n", count);
	} else if (compare_arrays(sensor_container, sensor_exit_pattern_0, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_exit_level_pattern_0, sizeof(&sensor_container))) {
		printf("Exit pattern 0 detected:\n");
		// debugging:
		print_array(sensor_container, 4);
         print_array(sensor_level_container, 4);
        printf("Pattern:\n");
        print_array(sensor_exit_pattern_0, 4);
		print_array(sensor_exit_level_pattern_0, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// substract person from room:
		count--;
        printf("count: %d\n", count);
	} else if (compare_arrays(sensor_container, sensor_exit_pattern_1, sizeof(&sensor_container)) && compare_arrays(sensor_level_container, sensor_exit_level_pattern_1, sizeof(&sensor_container))) {
		printf("Exit pattern 1 detected:\n");
		// debugging:
		print_array(sensor_container, 4);
		print_array(sensor_level_container, 4);
        printf("Pattern:\n");
        print_array(sensor_exit_pattern_1, 4);
		print_array(sensor_exit_level_pattern_1, 4);
		// after detection, reset arrays:
		reset_arrays(sensor_container, sensor_level_container, 4);
		// substract person from room!
		count--;
        printf("count: %d\n", count);
	} 
}

void insertSignalPinInToArrayBuffer(){ 
	shift_to_left(sensor_level_container, 4, gpio_get_level(triggerPinIn));
	shift_to_left(sensor_container, 4, triggerPinIn);
    detect_crossing_barrier_by_pattern();
}

void insertSignalPinOutToArrayBuffer(){ 
	shift_to_left(sensor_level_container, 4, gpio_get_level(triggerPinOut));
	shift_to_left(sensor_container, 4, triggerPinOut);
    detect_crossing_barrier_by_pattern();
}

			// check for pattern recognission:
			//detect_crossing_barrier_by_pattern(sensor_container, sensor_level_container);
			// debouncing delay
			//vTaskDelay(50 / portTICK_PERIOD_MS);


            // debugging functions:
            //printf("GPIO[%d] intr, val: %d, alr_setted: %d\n", io_num, gpio_get_level(io_num), gpio_4_level_set);
            // printf("GPIO[%d] interupt, val: %d \n", triggerPinIn, gpio_get_level(triggerPinIn));
