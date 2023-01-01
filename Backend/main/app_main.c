/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "cJSON.h"

#include "dht11.h"

static const char *TAG = "MQTT_EXAMPLE";
int Temp, Hum;
char str_temp[80], str_hum[80];
char my_json_string;
esp_mqtt_client_handle_t client;
int msg_id;
int rain;
static void read_DHT11()
{
    DHT11_init(9);
    Temp = DHT11_read().temperature;
    Hum = DHT11_read().humidity;
    rain = (rand() % (60 - 0 + 1));
    sprintf(str_temp, "%d", Temp);
    puts(str_temp);
    sprintf(str_hum, "%d", Hum);
    puts(str_hum);
}
// static void create_json()
// {
//     ESP_LOGI(TAG, "Serialize.....");
//     cJSON *root;
//     root = cJSON_CreateObject();
//     esp_chip_info_t chip_info;
//     esp_chip_info(&chip_info);
//     cJSON_AddStringToObject(root, "version", IDF_VER);
//     cJSON_AddNumberToObject(root, "cores", Temp);
//     cJSON_AddTrueToObject(root, "flag_true");
//     cJSON_AddFalseToObject(root, "flag_false");
//     // const char *my_json_string = cJSON_Print(root);
//     char *my_json_string = cJSON_Print(root);
//     ESP_LOGI(TAG, "my_json_string\n%s", my_json_string);
//     cJSON_Delete(root);
//     cJSON *root2 = cJSON_Parse(my_json_string);
//     if (cJSON_GetObjectItem(root2, "version"))
//     {
//         char *version = cJSON_GetObjectItem(root2, "version")->valuestring;
//         ESP_LOGI(TAG, "version=%s", version);
//     }
//     if (cJSON_GetObjectItem(root2, "cores"))
//     {
//         int cores = cJSON_GetObjectItem(root2, "cores")->valueint;
//         ESP_LOGI(TAG, "cores=%d", cores);
//     }
// }

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    // int msg_id;
    // cJSON *root;
    // root = cJSON_CreateObject();
    // cJSON_AddStringToObject(root, "Temp", str_temp);
    // cJSON_AddStringToObject(root, "Hum", str_hum);
    // char *my_json_string = cJSON_Print(root);

    // cJSON_Delete(root);
    // cJSON *root2 = cJSON_Parse(my_json_string);
    // char *string1 = cJSON_GetObjectItem(root2, "version")->valuestring;
    // char *Temp1 = cJSON_GetObjectItem(root2, "Temp")->valuestring;
    // char *Hum1 = cJSON_GetObjectItem(root2, "Hum")->valuestring;
    // int Hum1 = cJSON_GetObjectItem(root2, "Hum")->valueint;
    // sprintf(temp2, "Temp = %s", Temp1);
    // sprintf(hum2, "Temp = %s", Hum1);
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_subscribe(client, "test", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "test", my_json_string, 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:

        // msg_id = esp_mqtt_client_publish(client, "topic/subtopic", "121", 0, 0, 0);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://mqtt.eclipseprojects.io",

        // .port = 1883,
        // .username = "FlespiToken lCG8yJPUWRc9awe3M2AaTuKcqd5N4Nvgd1cByPklwkiuGmogcOgW6QWmURXOujSx",
        // .password = "",
        // .client_id = "mqtt-board-2d4f982e"
    };
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
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    // read_DHT11();
    // printf("Temperature is %d \n", Temp);
    // printf("Humidity is %d\n", Hum);

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    mqtt_app_start();
    ESP_ERROR_CHECK(example_connect());
    while (1)
    {
        read_DHT11();

        cJSON *root;
        root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "Temp", str_temp);
        cJSON_AddStringToObject(root, "Hum", str_hum);
        cJSON_AddNumberToObject(root, "Rain", rain);
        char *my_json_string = cJSON_Print(root);
        msg_id = esp_mqtt_client_publish(client, "test", my_json_string, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
