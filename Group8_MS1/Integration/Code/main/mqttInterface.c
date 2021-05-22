#include "mqttInterface.h"
#include "main_app.h"
#include "commands.h"

#include <time.h>
#include <sys/time.h>

static const char *TAG = "MQTT";
static const char *TAG2 = "MQTT-Platform";

static esp_mqtt_client_handle_t clientIOT;
static esp_mqtt_client_handle_t clientROOM;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "ROOM_EVENTS", 2);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        const char *room_events = "ROOM_EVENTS";

        int len = strlen(room_events);
        if ((len == event->topic_len) && (!strncmp(event->topic, room_events, len)))
        {
            if (event->data_len == 4 && (strncmp(event->data, "ping", 4) == 0))
            {
                ping();
            }
            else if (event->data_len == 5 && strncmp(event->data, "enter", 5) == 0)
            {
                //enterRoom();
                count++;
                mqttPublishCount();
            }
            else if (event->data_len == 5 && strncmp(event->data, "leave", 5) == 0)
            {
                //leaveRoom();
                if (count > 0)
                {
                    count--;
                }
                mqttPublishCount();
            }
            else if (event->data_len == 11 && strncmp(event->data, "unsureEnter", 11) == 0)
            {
                unsureEnter();
            }
            else if (event->data_len == 12 && strncmp(event->data, "halfwayEnter", 12) == 0)
            {
                halfwayEnter();
            }
            else if (event->data_len == 14 && strncmp(event->data, "personTurnedG9", 14) == 0)
            {
                personTurnedG9();
            }
            else if (event->data_len == 15 && strncmp(event->data, "successiveEnter", 15) == 0)
            {
                successiveEnter();
            }
            else if (event->data_len == 17 && strncmp(event->data, "manipulationEnter", 17) == 0)
            {
                manipulationEnter();
            }
            else if (event->data_len == 18 && strncmp(event->data, "peeketoandLeaveG11", 18) == 0)
            {
                peeketoandLeaveG11();
            }
            else if (event->data_len == 31 && strncmp(event->data, "breaksOuterAndInnerButReturnsG4", 31) == 0)
            {
                breaksOuterAndInnerButReturnsG4();
            }
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static esp_err_t mqtt_event_handler_cb2(esp_mqtt_event_handle_t event)
{
    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG2, "MQTT_EVENT_CONNECTED");
        /*
            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            */
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG2, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG2, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG2, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG2, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG2, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG2, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG2, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler2(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG2, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb2(event_data);
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };

    ESP_LOGI(TAG, CONFIG_BROKER_URL);

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0)
    {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128)
        {
            int c = fgetc(stdin);
            if (c == '\n')
            {
                line[count] = '\0';
                break;
            }
            else if (c > 0 && c < 127)
            {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    }
    else
    {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    clientROOM = client;

    //Second Client for data pushing
    esp_mqtt_client_config_t mqtt_cfg2 = {
        .uri = CONFIG_IOT_MQTT_ADDRESS,
        .port = CONFIG_IOT_MQTT_PORT,
        .username = CONFIG_IOT_MQTT_USERNAME,
        .password = CONFIG_IOT_MQTT_DEVICEKEY,
        .client_id = "platform-client",
    };

    client = esp_mqtt_client_init(&mqtt_cfg2);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler2, client);
    //esp_mqtt_client_start(client);
    clientIOT = client;
}

void mqttPublishCount()
{
    //Wait until time is synced before sending
    time_t now = 0;
    while (now < 1600000000)
    {
        time(&now);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    #ifndef TESTING
    esp_mqtt_client_start(clientIOT);
    ESP_LOGI(TAG2, "Sending count event");
    long long int now_Long = (long long) now;
    now_Long *= 1000LL;

    char buffer[256];
    int length = sprintf(buffer, "{\"username\":\"%s\",\"count\":%d,\"device_id\":%d,\"timestamp\":%lld}", CONFIG_IOT_USERNAME, count, CONFIG_IOT_DEVICEID, now_Long);
    int msg_id = esp_mqtt_client_publish(clientIOT, CONFIG_IOT_USER_DEVICEID, buffer, length, 2, 0);
    ESP_LOGI(TAG2, "sent publish successful, msg_id=%d", msg_id);
    printf(buffer);
    printf("\n");
    esp_mqtt_client_stop(clientIOT);
    #endif
}

void mqttPublishCountTask()
{
    //Wait until time is synced before sending
    time_t now = 0;
    time(&now);

    while (now < 1600000000)
    {
        time(&now);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    struct tm timeinfo = {0};
    //Run task every minute that sends count to iot platform at 00, 15, 30 and 45
    while (1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_min == 0 || timeinfo.tm_min == 15 || timeinfo.tm_min == 30 || timeinfo.tm_min == 45)
        {
            mqttPublishCount();
        }
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

void mqttPublishRestart()
{
    //Wait until time is synced before sending
    time_t now = 0;
    while (now < 1600000000)
    {
        time(&now);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    #ifndef TESTING
    esp_mqtt_client_start(clientIOT);
    ESP_LOGI(TAG2, "Sending restart event");
    long long int now_Long = (long long) now;
    now_Long *= 1000LL;

    char buffer[256];
    int length = sprintf(buffer, "{\"username\":\"%s\",\"restart\":1,\"device_id\":%d,\"timestamp\":%lld}", CONFIG_IOT_USERNAME, CONFIG_IOT_DEVICEID, now_Long);
    int msg_id = esp_mqtt_client_publish(clientIOT, CONFIG_IOT_USER_DEVICEID, buffer, length, 0, 0);
    ESP_LOGI(TAG2, "sent publish successful, msg_id=%d", msg_id);
    printf(buffer);
    printf("\n");
    esp_mqtt_client_stop(clientIOT);
    #endif
    //ESP_LOGI(TAG2, payload);
    vTaskDelete(NULL);
}