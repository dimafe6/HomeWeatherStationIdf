#include "main.h"
#include "monitoring.h"
#include "RF24.h"
#include "ds3231.h"

extern "C"
{
    void app_main();
}

const uint64_t pipes[5] = {0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

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

            printf("Temp: %d\n", externalSensor.temperature);
        }
    }
}

void ds3231_test(void *pvParameters)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, (gpio_num_t)CONFIG_BME280_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BME280_I2C_SCL_GPIO));

    struct tm time;

    while (1)
    {

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (ds3231_get_time(&dev, &time) != ESP_OK)
        {
            printf("Could not get time\n");
            continue;
        }

        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        printf("%04d-%02d-%02d %02d:%02d:%02d\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1,
               time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
    }
}

void app_main()
{
    vSemaphoreCreateBinary(xGlobalVariablesMutex);

    ESP_ERROR_CHECK(i2cdev_init());

    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore(task_test_nrf24, "task_test_nrf24", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL, APP_CPU_NUM);
    xTaskCreate(ds3231_test, "ds3231_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
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

    wifi_scan();    

    wifi_init_sta();

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