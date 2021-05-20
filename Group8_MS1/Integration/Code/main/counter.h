
#ifndef COUNTER_H
#define COUNTER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "main_app.h"

void test12();


// shifts the elements of an passed array 1 int to the left
static void shift_to_left(int *array, int size, int last_elem);

// prints passed array
static void print_array(int * array, int size);

// return true if passed arrays are equal
static bool compare_arrays(int * array1, int * array2, int size);

// sets all items in array to 0
static void reset_arrays(int * sensor_container, int * sensor_level_container, int size);

//detects etry or exit by pattern and counts room counter up or down respectively
static void detect_crossing_barrier_by_pattern(int *sensor_container, int *sensor_level_container);


#endif