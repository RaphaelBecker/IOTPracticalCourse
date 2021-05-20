#include "counter.h"


// #### pattern definition ####:
// enter_pattern_0:
uint8_t sensor_enter_pattern_0[4] = {4, 5, 4, 5};
uint8_t sensor_enter_level_pattern_0[4] = {1, 1, 0, 0};
// enter_pattern_1:
uint8_t sensor_enter_pattern_1[4] = {4, 4, 5, 5};
uint8_t sensor_enter_level_pattern_1[4] = {1, 0, 1, 0};
// exit  pattern_0:
uint8_t sensor_exit_pattern_0[4] = {5, 4, 5, 4};
uint8_t sensor_exit_level_pattern_0[4] = {1, 1, 0, 0};
// exit pattern_1:
uint8_t sensor_exit_pattern_1[4] = {5, 5, 4, 4};
uint8_t sensor_exit_level_pattern_1[4] = {1, 0, 1, 0};

//container for sensor signals
uint8_t sensor_level_container[4] = { 0 };
uint8_t sensor_container[4] = { 0 };

void test12()
{
    count++;
}

// shifts the elements of an passed array 1 int to the left
static void shift_to_left(int *array, int size, int last_elem){
	for(int i = 0; i < 3; i++) { 
		array[i] = array[i + 1]; 
	}
	array[size -1] = last_elem;
}

// prints passed array
static void print_array(int * array, int size) {
	for (int i = 0; i < size; i++) {
		printf("%d", array[i]);
	}
	printf("\n");
}

// return true if passed arrays are equal
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

// sets all items in array to 0
static void reset_arrays(int * sensor_container, int * sensor_level_container, int size) {
	for (int i = 0; i < size; i++) {
		sensor_container[i] = 0;
		sensor_level_container[i] = 0;
	}
}

static void detect_crossing_barrier_by_pattern(int *sensor_container, int *sensor_level_container){
	#ifdef BLEH
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
	#endif
}
