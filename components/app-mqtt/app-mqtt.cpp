#include "app-mqtt.h"

static const char *TAG = "MQTT";

esp_mqtt_client_handle_t mqtt_client = NULL;
static bool connected = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        connected = false;
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
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno", event->error_handle->error_type);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->error_type));
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && (event_id == WIFI_EVENT_STA_DISCONNECTED))
    {
        if (NULL != mqtt_client)
        {
            esp_mqtt_client_stop(mqtt_client);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        esp_mqtt_client_start(mqtt_client);
    }
}

void mqtt_app_init()
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL));

    esp_mqtt_client_config_t mqtt_cfg = {
        .host = CONFIG_MQTT_BROKER_HOST,
        .port = CONFIG_MQTT_BROKER_PORT,
        .username = CONFIG_MQTT_BROKER_USER,
        .password = CONFIG_MQTT_BROKER_PASS,
        .lwt_qos = 2,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void mqtt_pub(const char *topic, const char *data, int len, int qos, int retain)
{
    if (connected)
    {
        char topic_name[strlen(topic) + strlen(CONFIG_MQTT_BROKER_BASE_TOPIC) + 1];
        strcpy(topic_name, CONFIG_MQTT_BROKER_BASE_TOPIC);
        strcat(topic_name, topic);

        ESP_LOGI(TAG, "Sending data to the topic: %s", topic_name);
        int result = esp_mqtt_client_publish(mqtt_client, topic_name, data, len, qos, retain);
        if (result > -1)
        {
            ESP_LOGI(TAG, "Send data to the topic: %s successfully", topic_name);
        }
        else
        {
            ESP_LOGI(TAG, "Send data to the topic: %s failed", topic_name);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Cannot publish message to MQTT brocker!");
    }
}
