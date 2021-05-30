#include "app-mqtt.h"

static const char *TAG = "MQTT";

esp_mqtt_client_handle_t mqtt_client = NULL;
static bool connected = false;
unsigned long lastBMEDataSendToMQTT = INTERVAL_15_MIN;

static void mqtt_pub(const char *topic, const char *data, int len, int qos, int retain)
{
    if (connected)
    {
        ESP_LOGI(TAG, "Sending data to the topic: %s", topic);

        int result = esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
        if (result > -1)
        {
            ESP_LOGI(TAG, "Send data to the topic: %s successfully", topic);
        }
        else
        {
            ESP_LOGI(TAG, "Send data to the topic: %s failed", topic);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Cannot publish message to MQTT brocker, connection failed!");
    }
}

static void mqtt_pub_discovery(const char *component,
                               const char *component_config_key,
                               const char *device_class,
                               const char *component_name,
                               const char *state_topic,
                               const char *unit,
                               const char *value_template)
{
    char topic[strlen(component) + strlen(component_config_key) + 22];
    sprintf(topic, "homeassistant/%s/%s/config", component, component_config_key);

    char unique_id[strlen(mac_address) + strlen(component_config_key) + 6];
    sprintf(unique_id, "hwt-%s-%s", mac_address, component_config_key);

    char stopic[strlen(mac_address) + strlen(state_topic) + 6];
    sprintf(stopic, "hwt/%s/%s", mac_address, state_topic);

    char discovery[1024];
    sprintf(
        discovery,
        R"({"dev_cla":"%s","name":"%s","uniq_id":"%s","stat_t":"%s","unit_of_meas":"%s","val_tpl":"%s","qos":2,)"
        R"("dev":{"ids":"%s","connections":[["mac","%s"]],"mf": "Dmytro Feshchenko","name": "Home weather station","mdl":"Home weather station"}})",
        device_class,
        component_name,
        unique_id,
        stopic,
        unit,
        value_template,
        mac_address,
        mac_address);

    mqtt_pub(topic, discovery, 0, 2, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void mqtt_send_task(void *pvParameters)
{
    while (1)
    {
        char buf[100] = {0};
        sprintf(
            buf,
            R"({"temp":%2.2f,"hum":%2.2f,"dp":%2.2f,"hi":%i,"press":%i,"co2":%i})",
            internalSensorData.temperature,
            internalSensorData.humidity,
            internalSensorData.dewPoint,
            internalSensorData.humIndex,
            internalSensorData.pressureMmHg,
            internalSensorData.co2);
        mqtt_pub_sensor("indoor", buf);

        vTaskDelay(INTERVAL_10_SEC / portTICK_PERIOD_MS);
    }
}

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

        //MQTT discovery
        mqtt_pub_discovery("sensor", "indoor-temp", "temperature", "Indoor temperature", "indoor", "°C", "{{value_json.temp}}");
        mqtt_pub_discovery("sensor", "indoor-hum", "humidity", "Indoor humidity", "indoor", "%", "{{value_json.hum}}");
        mqtt_pub_discovery("sensor", "indoor-press", "pressure", "Indoor pressure", "indoor", "mmHg", "{{value_json.press}}");
        mqtt_pub_discovery("sensor", "indoor-dp", "temperature", "Indoor dew point", "indoor", "°C", "{{value_json.dp}}");
        mqtt_pub_discovery("sensor", "indoor-hi", "temperature", "Indoor heat index", "indoor", "°C", "{{value_json.hi}}");
        mqtt_pub_discovery("sensor", "indoor-co2", "carbon_dioxide", "Indoor CO₂", "indoor", "ppm", "{{value_json.co2}}");
        for (uint8_t i = 1; i <= CONFIG_APP_RF_SENSORS_COUNT; i++)
        {
            char topic[10];
            sprintf(topic, "outdoor/%i", i);

            char config_key[15];
            sprintf(config_key, "outdoor-%i-temp", i);
            char s_name[50];
            sprintf(s_name, "Outdoor %i temperature", i);
            mqtt_pub_discovery("sensor", config_key, "temperature", s_name, topic, "°C", "{{value_json.temp}}");

            config_key[0] = '\0';
            s_name[0] = '\0';
            sprintf(config_key, "outdoor-%i-hum", i);
            sprintf(s_name, "Outdoor %i humidity", i);
            mqtt_pub_discovery("sensor", config_key, "humidity", s_name, topic, "%", "{{value_json.hum}}");

            config_key[0] = '\0';
            s_name[0] = '\0';
            sprintf(config_key, "outdoor-%i-dp", i);
            sprintf(s_name, "Outdoor %i dew point", i);
            mqtt_pub_discovery("sensor", config_key, "temperature", s_name, topic, "°C", "{{value_json.dp}}");

            config_key[0] = '\0';
            s_name[0] = '\0';
            sprintf(config_key, "outdoor-%i-hi", i);
            sprintf(s_name, "Outdoor %i heat index", i);
            mqtt_pub_discovery("sensor", config_key, "temperature", s_name, topic, "°C", "{{value_json.hi}}");

            config_key[0] = '\0';
            s_name[0] = '\0';
            sprintf(config_key, "outdoor-%i-bat", i);
            sprintf(s_name, "Outdoor %i battery", i);
            mqtt_pub_discovery("sensor", config_key, "battery", s_name, topic, "%", "{{value_json.bat}}");
        }

        xTaskCreatePinnedToCore(mqtt_send_task, "mqtt_send_task", configMINIMAL_STACK_SIZE * 4, NULL, 2, NULL, APP_CPU_NUM);
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

void mqtt_pub_sensor(const char *topic, const char *data)
{
    char topic_name[strlen(topic) + strlen(mac_address) + 6];
    sprintf(topic_name, "hwt/%s/%s", mac_address, topic);
    mqtt_pub(topic_name, data, 0, 1, 0);
}