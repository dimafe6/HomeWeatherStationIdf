#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <BME280.h>
#include <string.h>
#include "nextion_display.h"
#include "nextion_text.h"
#include "esp_log.h"
#include <RF24.h>
#include "MH-Z19.h"
#include "bh1750.h"

extern "C"
{
    void app_main();
}

const nextion_display_t *display;
const nextion_text_t *text;

const uint64_t pipes[5] = {0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

typedef struct __attribute__((__packed__)) ExternalSensor
{
    int16_t temperature;
    int16_t humidity;
    uint8_t battery;
} ExternalSensor;

ExternalSensor externalSensor;

TaskHandle_t TaskHandle_xtask1;
TaskHandle_t TaskHandle_xtask2;
TaskHandle_t TaskHandle_xtask3;
TaskHandle_t TaskHandle_xtask4;

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

void bmp280_test(void *pvParameters)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev;
    memset(&dev, 0, sizeof(bmp280_t));

    ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, (gpio_num_t)CONFIG_BME280_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BME280_I2C_SCL_GPIO));
    ESP_ERROR_CHECK(bmp280_init(&dev, &params));

    bool bme280p = dev.id == BME280_CHIP_ID;
    printf("BMP280: found %s\n", bme280p ? "BME280" : "BMP280");

    float pressure, temperature, humidity;

    while (1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
        {
            printf("Temperature/pressure reading failed\n");
            continue;
        }

        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        printf("Pressure: %.2f Pa, Temperature: %.2f C", pressure, temperature);
        if (bme280p)
            printf(", Humidity: %.2f\n", humidity);
        else
            printf("\n");
    }
}

void task_test_co2(void *pvParameters)
{
    mhz19_init();

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        int co2 = mhz19_get_co2();

        printf("CO2: %d\n", co2);
    }
}

void task_test_light(void *pvParameters)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t)); // Zero descriptor

    ESP_ERROR_CHECK(bh1750_init_desc(&dev, BH1750_ADDR_LO, 0, (gpio_num_t)CONFIG_BH1750_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BH1750_I2C_SCL_GPIO));
    ESP_ERROR_CHECK(bh1750_setup(&dev, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH));

    while (1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);

        uint16_t lux;

        if (bh1750_read(&dev, &lux) != ESP_OK)
            printf("Could not read lux data\n");
        else
            printf("Lux: %d\n", lux);
    }
}

float GetTaskHighWaterMarkPercent( TaskHandle_t task_handle, uint32_t stack_allotment )
{
  UBaseType_t uxHighWaterMark;
  uint32_t diff;
  float result;

  uxHighWaterMark = uxTaskGetStackHighWaterMark( task_handle );

  diff = stack_allotment - uxHighWaterMark;

  result = ( (float)diff / (float)stack_allotment ) * 100.0;

  return result;
}

void app_main()
{
    /*display = (const nextion_display_t *)nextion_display_init(NULL);

    nextion_descriptor_t descriptor = {
        .page_id = 0,
        .component_id = 2,
        .name = "iTemp"};

    text = nextion_text_init(display, &descriptor);
*/
    ESP_ERROR_CHECK(i2cdev_init());

    xTaskCreatePinnedToCore(task_test_nrf24, "task_test_nrf24", configMINIMAL_STACK_SIZE * 3, NULL, 5, &TaskHandle_xtask1, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_test_co2, "task_test_co2", configMINIMAL_STACK_SIZE * 2, NULL, 1, &TaskHandle_xtask2, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_test_light, "task_test_light", configMINIMAL_STACK_SIZE * 3, NULL, 1, &TaskHandle_xtask3, APP_CPU_NUM);
    xTaskCreatePinnedToCore(bmp280_test, "bmp280_test", configMINIMAL_STACK_SIZE * 3, NULL, 1, &TaskHandle_xtask4, APP_CPU_NUM);

    /*while (1)
    {
        TickType_t xTime1 = xTaskGetTickCount();

        uint8_t temp1 = (uint8_t)GetTaskHighWaterMarkPercent(TaskHandle_xtask1, configMINIMAL_STACK_SIZE * 3);
        uint8_t temp2 = (uint8_t)GetTaskHighWaterMarkPercent(TaskHandle_xtask2, configMINIMAL_STACK_SIZE * 2);
        uint8_t temp3 = (uint8_t)GetTaskHighWaterMarkPercent(TaskHandle_xtask3, configMINIMAL_STACK_SIZE * 3);
        uint8_t temp4 = (uint8_t)GetTaskHighWaterMarkPercent(TaskHandle_xtask4, configMINIMAL_STACK_SIZE * 3);

        printf("\r\n************************************************\r\n");
        printf("Tick:                %06.1f\r\n", (float)xTime1 / 100.00);
        printf("task_test_nrf24:     %3u%%\r\n", temp1);
        printf("task_test_co2:       %3u%%\r\n", temp2);
        printf("task_test_light:     %3u%%\r\n", temp3);
        printf("bmp280_test:         %3u%%\r\n", temp4);

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }*/
}