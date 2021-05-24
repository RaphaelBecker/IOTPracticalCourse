#include "counter.h"
#include "roomMonitoring.h"

//triggerPinIn = 0;
//triggerPinOut = 2;

// #### pattern definition ####:
// enter_pattern_0:

uint8_t sensor_enter_pattern_0[4] = {triggerPinOut, triggerPinIn, triggerPinOut, triggerPinIn};
uint8_t sensor_enter_level_pattern_0[4] = {1, 1, 0, 0};
// enter_pattern_1:
uint8_t sensor_enter_pattern_1[4] = {triggerPinOut, triggerPinOut, triggerPinIn, triggerPinIn};
uint8_t sensor_enter_level_pattern_1[4] = {1, 0, 1, 0};
// exit  pattern_0:
uint8_t sensor_exit_pattern_0[4] = {triggerPinIn, triggerPinOut, triggerPinIn, triggerPinOut};
uint8_t sensor_exit_level_pattern_0[4] = {1, 1, 0, 0};
// exit pattern_1:
uint8_t sensor_exit_pattern_1[4] = {triggerPinIn, triggerPinIn, triggerPinOut, triggerPinOut};
uint8_t sensor_exit_level_pattern_1[4] = {1, 0, 1, 0};

//container for sensor signals
uint8_t sensor_level_container[4] = {255};
uint8_t sensor_container[4] = {255};
long long sensor_timestamps[4] = {0};

//Corner case catching
static void enterRegistered()
{
	//Wait 500ms
	vTaskDelay(500 / portTICK_PERIOD_MS);
	//Check if the outer sensor has been broken and unbroken in the meantime
	if (sensor_container[2] == triggerPinOut &&
		sensor_container[3] == triggerPinOut &&
		sensor_level_container[2] == 1 &&
		sensor_level_container[3] == 0)
	{
		printf("Person did in fact turn around\n");
	}
	else
	{
		// add person to room:
		if (count < 99)
		{
			count++;
		}
	}

	printf("count: %d\n", count);
}

static void exitRegistered()
{
	//Wait 500ms
	vTaskDelay(500 / portTICK_PERIOD_MS);
	//Check if the inner sensor has been broken and unbroken in the meantime
	if (sensor_container[2] == triggerPinIn &&
		sensor_container[3] == triggerPinIn &&
		sensor_level_container[2] == 1 &&
		sensor_level_container[3] == 0)
	{
		printf("Person did in fact turn around\n");
	}
	else
	{
		// substract person from room:
		if (count > 0)
		{
			count--;
		}
	}

	printf("count: %d\n", count);
}

// shifts the elements of an passed array 1 int to the left
static void shift_to_left(uint8_t *array, uint8_t size, uint8_t last_elem)
{
	for (int i = 0; i < size - 1; i++)
	{
		array[i] = array[i + 1];
	}
	array[size - 1] = last_elem;
}
static void shift_to_left_long(long long *array, uint8_t size, long long last_elem)
{
	for (int i = 0; i < size - 1; i++)
	{
		array[i] = array[i + 1];
	}
	array[size - 1] = last_elem;
}

//Interrupts are saved in the arrays
void insertSignalPinToArrayBuffer(int pin)
{

	int level = gpio_get_level((gpio_num_t)pin);

	//debugging:
	printf("GPIO[%d] interupt, val: %d \n", pin, level);

	shift_to_left(sensor_level_container, 4, level);
	shift_to_left(sensor_container, 4, pin);

	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	long long time = tv.tv_sec;
	time *= 1000;
	time += (tv.tv_usec/1000);

	printf("Timestamp: %lld \n", time);
	shift_to_left_long(sensor_timestamps, 4, time);
	detect_crossing_barrier_by_pattern();

	vTaskDelete(NULL);
}

// return true if passed arrays are equal
static bool compare_arrays(uint8_t *array1, uint8_t *array2, uint8_t size)
{
	for (int i = 0; i < size; i++)
	{
		if (array1[i] != array2[i])
		{
			return false;
		}
	}
	return true;
}

// sets all items in array to 255
void reset_arrays()
{
	for (int i = 0; i < 4; i++)
	{
		sensor_container[i] = 255;
		sensor_level_container[i] = 255;
		sensor_timestamps[i] = 0;
	}
}

static long long calculateTimeForPattern()
{
	return (sensor_timestamps[3] - sensor_timestamps[0]);
}

//Return true if timestamps are too close
static bool checkTimeForManipulation()
{
	long long patternTotalTicks = calculateTimeForPattern();
	//Time manipulation is not a thing if there are no 4 timestamps in buffer
	if (sensor_timestamps[0] == 0)
	{
		return false;
	}
	//Check if complete pattern takes less than 150ms
	if (patternTotalTicks < 150)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void detect_crossing_barrier_by_pattern()
{
	if (checkTimeForManipulation())
	{
		printf("Manipulation Detected: %lld\n",calculateTimeForPattern());
		//TODO
		//reset_arrays();
		return;
	}
	//If pattern takes longer than 1.5 seconds we stop
	if (calculateTimeForPattern()>1500)
	{
		printf("Pattern time too long\n");
		return;
	}

	// checks for enter/exit pattern 0/1
	if (compare_arrays(sensor_container, sensor_enter_pattern_0, 4) &&
		compare_arrays(sensor_level_container, sensor_enter_level_pattern_0, 4))
	{
		printf("Enter pattern 0 detected:\n");

		// after detection, reset arrays:
		reset_arrays();
		enterRegistered();
	}
	else if (compare_arrays(sensor_container, sensor_enter_pattern_1, 4) &&
			 compare_arrays(sensor_level_container, sensor_enter_level_pattern_1, 4))
	{
		printf("Enter pattern 1 detected:\n");
		// debugging:
		//print_array(sensor_container, 4);
		//print_array(sensor_level_container, 4);
		//printf("Pattern:\n");
		//print_array(sensor_enter_pattern_1, 4);
		//print_array(sensor_enter_level_pattern_1, 4);
		// after detection, reset arrays:
		reset_arrays();

		enterRegistered();
	}
	else if (compare_arrays(sensor_container, sensor_exit_pattern_0, 4) &&
			 compare_arrays(sensor_level_container, sensor_exit_level_pattern_0, 4))
	{
		printf("Exit pattern 0 detected:\n");
		// debugging:
		//print_array(sensor_container, 4);
		//print_array(sensor_level_container, 4);
		//printf("Pattern:\n");
		//print_array(sensor_exit_pattern_0, 4);
		//print_array(sensor_exit_level_pattern_0, 4);
		// after detection, reset arrays:
		reset_arrays();
		exitRegistered();
	}
	else if (compare_arrays(sensor_container, sensor_exit_pattern_1, 4) &&
			 compare_arrays(sensor_level_container, sensor_exit_level_pattern_1, 4))
	{
		printf("Exit pattern 1 detected:\n");
		// debugging:
		//print_array(sensor_container, 4);
		//print_array(sensor_level_container, 4);
		//printf("Pattern:\n");
		//print_array(sensor_exit_pattern_1, 4);
		//print_array(sensor_exit_level_pattern_1, 4);
		// after detection, reset arrays:
		reset_arrays();
		// substract person from room!
		exitRegistered();
	}
	else
	{
		printf("no pattern detected\n");
	}
}

// check for pattern recognission:
//detect_crossing_barrier_by_pattern(sensor_container, sensor_level_container);
// debouncing delay
//vTaskDelay(50 / portTICK_PERIOD_MS);

// debugging functions:
//printf("GPIO[%d] intr, val: %d, alr_setted: %d\n", io_num, gpio_get_level(io_num), gpio_4_level_set);
// printf("GPIO[%d] interupt, val: %d \n", triggerPinIn, gpio_get_level(triggerPinIn));

/*DEBUG*/

// Debug: prints passed array
void print_array(uint8_t *array, uint8_t size)
{
	for (int i = 0; i < size; i++)
	{
		printf("%d\t", array[i]);
	}
	printf("\n");
}
