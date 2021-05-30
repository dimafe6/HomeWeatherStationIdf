#include "main.h"
#include "monitoring.h"

extern "C"
{
    void app_main();
    static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
}

static const char *TAG = "Main";

void app_main()
{
    vSemaphoreCreateBinary(xGlobalVariablesMutex);

    ESP_ERROR_CHECK(i2cdev_init());

    if (!obtain_time_from_rtc())
    {
        ESP_LOGW(TAG, "Could not get time from RTC.");
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uint8_t mac_bytes[6];
    esp_read_mac(mac_bytes, ESP_MAC_WIFI_STA);
    sprintf(mac_address, "%02x-%02x-%02x-%02x-%02x-%02x", mac_bytes[0], mac_bytes[1], mac_bytes[2], mac_bytes[3], mac_bytes[4], mac_bytes[5]);

    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore(display_task, "display_task", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(nrf24_task, "nrf24_task", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bme280_task, "bme280_task", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(mhz19_task, "mhz19_task", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bh1750_task, "bh1750_task", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, APP_CPU_NUM);

    wifi_init_sta(CONFIG_DEFAULT_WIFI_SSID, CONFIG_DEFAULT_WIFI_PASS);
    mqtt_app_init();

    /*while (1)
    {
        TickType_t xTime1 = xTaskGetTickCount();

        uint8_t temp1 = (uint8_t)GetTaskHighWaterMarkPercent(xHandle, configMINIMAL_STACK_SIZE * 3);

        printf("\r\n************************************************\r\n");
        printf("Tick:            %06.1f\r\n", (float)xTime1 / 100.00);
        printf("bh1750_task:     %3u%%\r\n", temp1);

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }*/
}