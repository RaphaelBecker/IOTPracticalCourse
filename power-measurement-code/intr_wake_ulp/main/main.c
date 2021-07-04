#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_pm.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc.h"
#include "driver/rtc_io.h"
#include "driver/rtc_cntl.h"
#include "esp32/ulp.h"
#include "sdkconfig.h"
#include "time.h"
#include <sys/time.h>

#include "deep_sleep.h"

#include "ulp_main.h"
#include "ulp_wake.h"
#include "network_common.h"
#include "sntpController.h"
#include "display.h"

static int counting(gpio_evt_msg *message);
static void gpio_task();
static void send_task();
static void ulp_isr(void *arg);
static void realtime_now(int64_t *timestamp_ms);

struct CountValue
{
    int64_t timestamp_ms;
    int count;
};
enum
{
    COUNT_DATA_N_MAX = 20
};
static RTC_DATA_ATTR int count_data_n = 0;
static RTC_DATA_ATTR struct CountValue count_data[COUNT_DATA_N_MAX];
static RTC_NOINIT_ATTR int count;

static const char *TAG = "main";
static SemaphoreHandle_t sema = NULL;

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void gpio_task()
{
    gpio_evt_msg msg;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &msg, portMAX_DELAY))
        {

            printf("event time: %lld pin: %d level %d\n", msg.timestamp_10us, msg.rtc_gpio_num, msg.level);
            counting(&msg);
        }
    }
}

// #### pattern definition ####:
// enter_pattern_0:

#define OUTERGPIOPIN 5//GPIO_NUM_35
#define INNERGPIOPIN 4//GPIO_NUM_34

int16_t sensor_enter_pattern_0[4] = {OUTERGPIOPIN, INNERGPIOPIN, OUTERGPIOPIN, INNERGPIOPIN};
int16_t sensor_enter_level_pattern_0[4] = {1, 1, 0, 0};
// enter_pattern_1:
int16_t sensor_enter_pattern_1[4] = {OUTERGPIOPIN, OUTERGPIOPIN, INNERGPIOPIN, INNERGPIOPIN};
int16_t sensor_enter_level_pattern_1[4] = {1, 0, 1, 0};
// exit  pattern_0:
int16_t sensor_exit_pattern_0[4] = {INNERGPIOPIN, OUTERGPIOPIN, INNERGPIOPIN, OUTERGPIOPIN};
int16_t sensor_exit_level_pattern_0[4] = {1, 1, 0, 0};
// exit pattern_1:
int16_t sensor_exit_pattern_1[4] = {INNERGPIOPIN, INNERGPIOPIN, OUTERGPIOPIN, OUTERGPIOPIN};
int16_t sensor_exit_level_pattern_1[4] = {1, 0, 1, 0};

//container for sensor signals
int16_t sensor_level_container[4] = {255};
int16_t sensor_container[4] = {255};
long long sensor_timestamps[4] = {0};

// Debug: prints passed array
static void print_array(int16_t *array, uint8_t size)
{
	for (int i = 0; i < size; i++)
	{
		printf("%d\t", array[i]);
	}
	printf("\n");
}

//Corner case catching
static void enterRegistered(int64_t timestamp)
{
	//Wait 500ms
	vTaskDelay(500 / portTICK_PERIOD_MS);
	//Check if the outer sensor has been broken and unbroken in the meantime
	if (sensor_container[2] == OUTERGPIOPIN &&
		sensor_container[3] == OUTERGPIOPIN &&
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
            //count_changes(timestamp, count);
		}
        
	}

	printf("Count++: %d\n", count);
}

static void exitRegistered(int64_t timestamp)
{
	//Wait 500ms
	vTaskDelay(500 / portTICK_PERIOD_MS);
	//Check if the inner sensor has been broken and unbroken in the meantime
	if (sensor_container[2] == INNERGPIOPIN &&
		sensor_container[3] == INNERGPIOPIN &&
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
            //count_changes(timestamp, count);
		}
        
	}

	printf("Count--: %d\n", count);
}


// shifts the elements of an passed array 1 int to the left
static void shift_to_left(int16_t *array, uint8_t size, int16_t last_elem)
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

// return true if passed arrays are equal
static bool compare_arrays(int16_t *array1, int16_t *array2, uint8_t size)
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
    //printf("Pattern from %lld to %lld\n", sensor_timestamps[0], sensor_timestamps[3]);
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

static void detect_crossing_barrier_by_pattern(int64_t timestamp)
{
	if (checkTimeForManipulation())
	{
		printf("Manipulation Detected: %lld\n",calculateTimeForPattern());
		return;
	}
	//If pattern takes longer than 1.5 seconds we stop
    long long timeToCompletion = calculateTimeForPattern();
	if (timeToCompletion>1500)
	{
		printf("Pattern time too long: %lld\n",timeToCompletion);
		return;
	}

	// checks for enter/exit pattern 0/1
	if (compare_arrays(sensor_container, sensor_enter_pattern_0, 4) &&
		compare_arrays(sensor_level_container, sensor_enter_level_pattern_0, 4))
	{
		printf("Enter pattern 0 detected:\n");

		// after detection, reset arrays:
		reset_arrays();
		enterRegistered(timestamp);
	}
	else if (compare_arrays(sensor_container, sensor_enter_pattern_1, 4) &&
			 compare_arrays(sensor_level_container, sensor_enter_level_pattern_1, 4))
	{
		printf("Enter pattern 1 detected:\n");
		reset_arrays();

		enterRegistered(timestamp);
	}
	else if (compare_arrays(sensor_container, sensor_exit_pattern_0, 4) &&
			 compare_arrays(sensor_level_container, sensor_exit_level_pattern_0, 4))
	{
		printf("Exit pattern 0 detected:\n");
		reset_arrays();
		exitRegistered(timestamp);
	}
	else if (compare_arrays(sensor_container, sensor_exit_pattern_1, 4) &&
			 compare_arrays(sensor_level_container, sensor_exit_level_pattern_1, 4))
	{
		printf("Exit pattern 1 detected:\n");
		reset_arrays();
		exitRegistered(timestamp);
	}
	else
	{
		printf("no pattern detected\n");
	}
}

static int counting(gpio_evt_msg *message)
{
    //TODO: intergrate your cornercase handler

    shift_to_left(sensor_level_container, 4, message->level);
	shift_to_left(sensor_container, 4, message->rtc_gpio_num);
    shift_to_left_long(sensor_timestamps, 4, message->timestamp_10us/100LL);

	print_array(sensor_level_container, 4);
	print_array(sensor_container, 4);

    detect_crossing_barrier_by_pattern(message->timestamp_10us/100LL);

    return 0;
}

static void realtime_now(int64_t *timestamp_ms)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    *timestamp_ms = (now.tv_sec * (int64_t)1000) + (now.tv_usec / 1000);
}

static void ulp_isr(void *arg)
{
    portBASE_TYPE pxHigherPriorityTaskWoken = 0;
    xSemaphoreGiveFromISR(sema, &pxHigherPriorityTaskWoken);
}

static void send_task()
{
    while (1)
    {
        if (xSemaphoreTake(sema, portMAX_DELAY))
        {
            restart_sleep_timer();
            send_gpio_evt();
        }
    }
}

void app_main(void)
{
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero\n");
        count = 0;
        /*start_wifi();
        initializeSntp();
        obtainTime();*/
        init_ulp_program();
    }
    sema = xSemaphoreCreateBinary();
    assert(sema);

    create_ulp_interface_queue();
    xTaskCreatePinnedToCore(gpio_task, "gpio task", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(send_task, "read rtc data", 2048, NULL, 2, NULL, 1);
    send_gpio_evt();
    esp_err_t err = rtc_isr_register(&ulp_isr, NULL, RTC_CNTL_SAR_INT_ST_M);
    ESP_ERROR_CHECK(err);
    REG_SET_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_ULP_CP_INT_ENA_M);

    create_sleep_timer();
    initDisplay();
    char text[20];
    while (1)
    {
        sprintf(text, "count %d", count);
        displayText(text);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
