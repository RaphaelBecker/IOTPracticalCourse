#include "http_client.h"
#include "main_app.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"

#include "esp_http_client.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "iotplatform.caps.in.tum.de"
#define WEB_PORT "443"
#define SENSOR_ID CONFIG_SENSOR_ID
#define WEB_URL "https://iotplatform.caps.in.tum.de:443/api/consumers/consume/" SENSOR_ID "/_search"
#define WEB_CONSUMER_TOKEN CONFIG_WEB_CONSUMER_TOKEN

static const char *TAG = "HTTP_CLIENT";

long long last_timestamp = 0;
uint8_t last_count = 0;

#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_HTTP_OUTPUT_BUFFER 2048

char *JSON_Types(int type)
{
    if (type == cJSON_Invalid)
        return ("cJSON_Invalid");
    if (type == cJSON_False)
        return ("cJSON_False");
    if (type == cJSON_True)
        return ("cJSON_True");
    if (type == cJSON_NULL)
        return ("cJSON_NULL");
    if (type == cJSON_Number)
        return ("cJSON_Number");
    if (type == cJSON_String)
        return ("cJSON_String");
    if (type == cJSON_Array)
        return ("cJSON_Array");
    if (type == cJSON_Object)
        return ("cJSON_Object");
    if (type == cJSON_Raw)
        return ("cJSON_Raw");
    return NULL;
}

void JSON_GetLatestCount(const cJSON *const root)
{
    //ESP_LOGI(TAG, "root->type=%s", JSON_Types(root->type));
    cJSON *current_element = NULL;
    //ESP_LOGI(TAG, "roo->child=%p", root->child);
    //ESP_LOGI(TAG, "roo->next =%p", root->next);

    cJSON_ArrayForEach(current_element, root)
    {
        if (current_element->string)
        {
            const char *name = current_element->string;
            //ESP_LOGI(TAG, "[%s]", name);
            if (strcmp(name, "timestamp") == 0)
            {
                if (cJSON_IsNumber(current_element))
                {
                    last_timestamp = (long long)current_element->valuedouble;
                }
            }
            else if (strcmp(name, "value") == 0)
            {
                if (cJSON_IsNumber(current_element))
                {
                    last_count = (uint8_t)current_element->valueint;
                }
            }
        }
        if (cJSON_IsArray(current_element))
        {
            //ESP_LOGI(TAG, "Array");
            JSON_GetLatestCount(current_element);
        }
        else if (cJSON_IsObject(current_element))
        {
            //ESP_LOGI(TAG, "Object");
            JSON_GetLatestCount(current_element);
        }
        else if (cJSON_IsRaw(current_element))
        {
            ESP_LOGI(TAG, "Raw(Not support)");
        }
    }
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            //ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

static void http_rest_with_url()
{
    while (1)
    {

        char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
        /**
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
     * If host and path parameters are not set, query parameter will be ignored. In such cases,
     * query parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     */
        esp_http_client_config_t config = {
            .host = WEB_SERVER,
            .path = "/api/consumers/consume/" SENSOR_ID "/_search",
            //.query = "esp",
            .event_handler = _http_event_handler,
            .user_data = local_response_buffer, // Pass address of local buffer to get response
            .buffer_size_tx = 2048,
            .buffer_size = 4096,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        // POST
        const char *post_data = "{\"_source\": [\"value\",\"timestamp\"],\"sort\":[{\"timestamp\": {\"order\": \"desc\"}}],\"size\":1}";
        esp_http_client_set_url(client, "https://iotplatform.caps.in.tum.de:443/api/consumers/consume/" SENSOR_ID "/_search");
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Authorization", "Bearer " WEB_CONSUMER_TOKEN);
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            int bufferSize = esp_http_client_get_content_length(client);
            char *buffer = malloc(bufferSize + 1);
            ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                     esp_http_client_get_status_code(client),
                     bufferSize);
            esp_http_client_read_response(client, buffer, bufferSize);
            ESP_LOGI(TAG, "buffer=\n%s", buffer);

            ESP_LOGI(TAG, "Deserialize.....");
            cJSON *root = cJSON_Parse(buffer);
            JSON_GetLatestCount(root);
            cJSON_Delete(root);
            free(buffer);

            time_t now = 0;
            while (now < 1600000000)
            {
                time(&now);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            long long int now_Long = (long long)now;
            now_Long *= 1000LL;

            //if last pushed count is recent (not older than 35 minutes, so we can miss 2 periodic pushes in theory)
            if (now_Long - last_timestamp < 2100000)
            {
                ESP_LOGI(TAG, "Last count is used to initialize counter");
                count = last_count;
            }

            break;
        }
        else
        {
            ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void checkAPIforLatestCount(void)
{
    xTaskCreate(http_rest_with_url, "http_rest_with_url", 16384, NULL, 5, NULL);
}