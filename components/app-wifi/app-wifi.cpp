#include <stdio.h>
#include "app-wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 10

static const char *TAG = "WiFi";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group = NULL;
bool initialized = false;

void wifi_reconnect()
{
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    esp_wifi_connect();
    ESP_LOGI(TAG, "retry to connect to the AP");
    ESP_LOGI(TAG, "connect to the AP fail");
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && (event_id == WIFI_EVENT_STA_DISCONNECTED))
    {
        wifi_reconnect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        obtain_time_from_ntp();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP)
    {
        wifi_reconnect();
    }
}

void wifi_init(void)
{
    if (initialized)
    {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        NULL));

    initialized = true;
}

void wifi_scan(void)
{
    wifi_init();

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
    {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }
}

void wifi_init_sta(const char *ssid, const char *password)
{
    wifi_init();

    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    if (NULL != ssid && NULL != password)
    {
        memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK)
    {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "failed to connect");
        return;
    }

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

bool wifi_connected()
{
    if (!s_wifi_event_group)
    {
        return false;
    }

    return xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT;
}