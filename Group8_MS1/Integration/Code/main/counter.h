
#ifndef COUNTER_H
#define COUNTER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "main_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

void test12();


// shifts the elements of an passed array 1 int to the left
void shift_to_left(uint8_t *array, uint8_t size, uint8_t last_elem);

// prints passed array
void print_array(uint8_t * array, uint8_t size);

// return true if passed arrays are equal
bool compare_arrays(uint8_t * array1, uint8_t * array2, uint8_t size);

// sets all items in array to 0
void reset_arrays(uint8_t * sensor_container, uint8_t * sensor_level_container, uint8_t size);

//detects etry or exit by pattern and counts room counter up or down respectively
void detect_crossing_barrier_by_pattern();

//inserts Interrupt signals to array buffer
void insertSignalPinInToArrayBuffer();

void insertSignalPinOutToArrayBuffer();
#endif