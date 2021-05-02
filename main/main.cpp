#include "main.h"
#include "monitoring.h"
#include "RF24.h"

extern "C"
{
    void app_main();
}

static const uint64_t pipes[5] = {0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

static const char *TAG = "Main";

void task_test_nrf24(void *pvParameters)
{
    printf("\nStart");
    RF24 radio(2, 4);
    radio.begin();
    radio.printDetails();
    radio.setAutoAck(false);
    radio.setChannel(80);
    radio.disableCRC();
    radio.setPayloadSize(5);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.maskIRQ(1, 1, 0);

    radio.openWritingPipe(0xF0F0F0F0AALL);
    radio.openReadingPipe(1, pipes[0]);
    radio.openReadingPipe(2, pipes[1]);
    radio.openReadingPipe(3, pipes[2]);
    radio.openReadingPipe(4, pipes[3]);
    radio.openReadingPipe(5, pipes[4]);

    radio.startListening();
    while (true)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);

        uint8_t pipeNum = 0;
        if (radio.available(&pipeNum))
        {
            radio.read(&externalSensor, sizeof(externalSensor.temperature));

            printf("Temp(%d): %d\n", pipeNum, externalSensor.temperature);
        }
    }
}

void app_main()
{
    vSemaphoreCreateBinary(xGlobalVariablesMutex);

    ESP_ERROR_CHECK(i2cdev_init());

    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore(task_test_nrf24, "task_test_nrf24", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bme280_task, "bme280_task", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(mhz19_task, "mhz19_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bh1750_task, "bh1750_task", configMINIMAL_STACK_SIZE * 3, NULL, 1, &xHandle, APP_CPU_NUM);
    xTaskCreatePinnedToCore(display_task, "display_task", configMINIMAL_STACK_SIZE * 3, NULL, 4, NULL, APP_CPU_NUM);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);    

    if (!obtain_time_from_rtc())
    {
        ESP_LOGW(TAG, "Could not get time from RTC.");
    }

    set_timezone("EET-2EEST,M3.5.0/3,M10.5.0/4"); //TODO: Configure from display

    wifi_scan();

    wifi_init_sta(CONFIG_DEFAULT_WIFI_SSID, CONFIG_DEFAULT_WIFI_PASS);

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