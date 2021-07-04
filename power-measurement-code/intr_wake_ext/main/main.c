#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_pm.h"

#include "edge_intr_debounce.h"
#include "commands_corner_cases.h"
#include "sntpController.h"
#include "network_common.h"
#include "deep_sleep.h"
#include "sdkconfig.h"
#include "display.h"

static int counting(gpio_evt_msg message);
static void count_changes(int64_t timestamp_ms, int count);
static void pm_config();
static void gpio_task(void *_);

#ifdef DEEP_SLEEP
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
#endif

static const char *TAG = "main";
static RTC_NOINIT_ATTR int count;

static esp_mqtt_client_handle_t client = NULL;

/*task to receive edge intrrupt messages*/
static void gpio_task(void *_)
{
    gpio_evt_msg msg;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &(msg), portMAX_DELAY))
        {
#ifdef DEEP_SLEEP
            restart_sleep_timer();
#endif
            printf("now ms: %lld\n", msg.timestamp);
            printf("gpio interrupt on pin %d, val %d\n", msg.pin, msg.level);

            counting(msg);
        }
    }
}

// #### pattern definition ####:
// enter_pattern_0:

#define OUTERGPIOPIN 25
#define INNERGPIOPIN 4

int32_t sensor_enter_pattern_0[4] = {OUTERGPIOPIN, INNERGPIOPIN, OUTERGPIOPIN, INNERGPIOPIN};
int32_t sensor_enter_level_pattern_0[4] = {1, 1, 0, 0};
// enter_pattern_1:
int32_t sensor_enter_pattern_1[4] = {OUTERGPIOPIN, OUTERGPIOPIN, INNERGPIOPIN, INNERGPIOPIN};
int32_t sensor_enter_level_pattern_1[4] = {1, 0, 1, 0};
// exit  pattern_0:
int32_t sensor_exit_pattern_0[4] = {INNERGPIOPIN, OUTERGPIOPIN, INNERGPIOPIN, OUTERGPIOPIN};
int32_t sensor_exit_level_pattern_0[4] = {1, 1, 0, 0};
// exit pattern_1:
int32_t sensor_exit_pattern_1[4] = {INNERGPIOPIN, INNERGPIOPIN, OUTERGPIOPIN, OUTERGPIOPIN};
int32_t sensor_exit_level_pattern_1[4] = {1, 0, 1, 0};

//container for sensor signals
int32_t sensor_level_container[4] = {255};
int32_t sensor_container[4] = {255};
long long sensor_timestamps[4] = {0};

//Corner case catching
static void enterRegistered(int64_t timestamp)
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
            count_changes(timestamp, count);
		}
        
	}

	printf("Count++: %d\n", count);
}

static void exitRegistered(int64_t timestamp)
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
            count_changes(timestamp, count);
		}
        
	}

	printf("Count--: %d\n", count);
}


// shifts the elements of an passed array 1 int to the left
static void shift_to_left(int32_t *array, uint8_t size, int32_t last_elem)
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
static bool compare_arrays(int32_t *array1, int32_t *array2, uint8_t size)
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

static int counting(gpio_evt_msg message)
{
    //TODO: intergrate your cornercase handler

    shift_to_left(sensor_level_container, 4, message.level);
	shift_to_left(sensor_container, 4, message.pin);
    shift_to_left_long(sensor_timestamps, 4, message.timestamp);

    detect_crossing_barrier_by_pattern(message.timestamp);

    return 0;
#ifdef SAMPLECODE
    //following is a sample code:
    static int8_t transition_table[][4] = {
        //event: pin1-fall pin1-rise pin2-fall pin2-rise
        {-1, 1, -1, 2},  //state 0: neither high
        {0, -1, -1, 3},  //state 1: pin1 high
        {-1, 3, 0, -1},  //state 2: pin2 high
        {2, -1, 1, -1}}; //state 3: both high
    static int8_t action_table[][2] = {
        //fall rise
        {0, 1},  //pin1
        {2, 3}}; //pin2

    static int state = 0;
    static uint32_t history = 0;

    int pin = (message.pin == GPIO_PIN_1) ? 0 : 1;
    int action = action_table[pin][message.level];
    state = transition_table[state][action];
    if (state < 0)
    {
        printf("impossible action, debouncing has failed\n");
        state = 0;
        return -1;
    }
    history *= 10;
    history += state;
    history %= 10000;
    printf("history: %d\n", history);
    if (history == 1020)
    {
        count++;
        count_changes(message.timestamp, count);
    }
    else if (history == 2010)
    {
        count--;
        count_changes(message.timestamp, count);
    }
    return 0;
#endif
}

static void count_changes(int64_t timestamp_ms, int count)
{
#ifndef DEEP_SLEEP
    size_t buf_len = 40;
    char buf[buf_len];
    snprintf(buf, buf_len, "timestamp %lld: count %d\n", timestamp_ms, count);
    if (client != NULL)
    {
        mqtt_send(client, "test", buf);
    }
    
#else
    if (count_data_n >= COUNT_DATA_N_MAX)
    {
        ESP_LOGW(TAG, "not enough space to store");
        return;
    }
    count_data[count_data_n].timestamp_ms = timestamp_ms;
    count_data[count_data_n].count = count;
    count_data_n += 1;
#endif
    return;
}

#ifdef DEEP_SLEEP
void publish_rtc_data(void)
{
    /*use snprintf to avoid buffer overflow, 
    change buffer length if the message becomes longer*/
    size_t buf_len = 40;
    char buf[buf_len];
    for (int i = 0; i < count_data_n; i++)
    {
        snprintf(buf, buf_len, "time: %lld, count: %d", count_data[i].timestamp_ms, count_data[i].count);
        if (client != NULL)
        {
            mqtt_send(client, "test", buf);
        }
    }
    count_data_n = 0;
}
#endif

#ifndef DEEP_SLEEP
void app_main(void)
{
    pm_config();
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero\n");
        count = 0;
    }
    start_wifi();
    initializeSntp();
    obtainTime();
    //start_mqtt(&client);

    wakeup_setup();
    xTaskCreatePinnedToCore(gpio_task, "gpio task", 4000, NULL, 1, NULL, 1);
    simulation_setup();
    //TaskCreatePinnedToCore(dump_pm, "dump pm", 2048, NULL, 1, NULL, 0);
    initDisplay();
    char text[20];
    while (1)
    {
        sprintf(text, "count %d", count);
        displayText(text);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#else
void app_main(void)
{
    wakeup_setup();
    pm_config();

    xTaskCreatePinnedToCore(gpio_task, "gpio task", 4000, NULL, 1, NULL, 1);
    simulation_setup();
    print_slept_time();
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero");
        count = 0;
        start_wifi();
        initializeSntp();
        obtainTime();
    }

    if ((esp_reset_reason() == ESP_RST_DEEPSLEEP) && (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER))
    {
        if (count_data_n)
        {
            start_wifi();
            start_mqtt(&client);
            publish_rtc_data();
        }
        else
        {
            printf("nothing to publish\n");
        }
    }
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
#endif



static void pm_config()
{
#if CONFIG_PM_ENABLE
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
    esp_pm_config_esp32_t pm_config = {.max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
                                       .min_freq_mhz = CONFIG_ESP32_XTAL_FREQ,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
#ifndef DEEP_SLEEP
                                       .light_sleep_enable = true,
#else
                                       .light_sleep_enable = false,
#endif
#endif
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif // CONFIG_PM_ENABLE
}
void dump_pm(void *_)
{
    while (1)
    {
        esp_pm_dump_locks(stdout);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}