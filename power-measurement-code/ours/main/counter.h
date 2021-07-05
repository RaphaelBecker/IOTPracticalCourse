
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




// prints passed array
void print_array(uint8_t * array, uint8_t size);

// sets all items in array to 0
void reset_arrays();

//detects etry or exit by pattern and counts room counter up or down respectively
void detect_crossing_barrier_by_pattern();

//inserts Interrupt signals to array buffer PinIn
void insertSignalPinToArrayBuffer(int pin);

//inserts 3 to buffers in case of the timeWatchDog in counter.c detects a very short time between PinIn or PinOut signals
void insertManipulationFlagToArrayBuffer();
#endif