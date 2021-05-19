#include "app-rf.h"

unsigned long lastExternalTemperatureHistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastExternalHumidityHistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastExternalTemperatureHistoryOneHourUpdateTime = INTERVAL_1_HOUR;
unsigned long lastExternalHumidityHistoryOneHourUpdateTime = INTERVAL_1_HOUR;

static const uint64_t pipes[5] = {0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

static const char *TAG = "RF";

void nrf24_task(void *pvParameters)
{
    RF24 radio(CONFIG_APP_RF_CE_PIN, CONFIG_APP_RF_CSN_PIN);
    radio.begin();
    radio.printDetails();
    radio.setAutoAck(false);
    radio.setChannel(CONFIG_APP_RF_CHANNEL);
    radio.disableCRC();
    radio.setPayloadSize(5);
    radio.setPALevel(CONFIG_APP_RF_PA_LEVEL);
    radio.setDataRate((rf24_datarate_e)CONFIG_APP_RF_DATA_RATE);
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

        uint8_t pipeNum = 1;
        if (radio.available(&pipeNum))
        {

            if (xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY) == pdTRUE)
            {
                radio.read(&externalSensor, sizeof(externalSensor));

                prevExternalSensorData[pipeNum - 1] = externalSensorData[pipeNum - 1];

                externalSensorData[pipeNum - 1].sensorId = pipeNum;
                externalSensorData[pipeNum - 1].battery = externalSensor.battery;
                externalSensorData[pipeNum - 1].signal = radio.testRPD();
                externalSensorData[pipeNum - 1].humidity = float(externalSensor.humidity) / 100;

                if (externalSensorData[pipeNum - 1].humidityMin == NULL)
                {
                    externalSensorData[pipeNum - 1].humidityMin = externalSensorData[pipeNum - 1].humidity;
                }
                else
                {
                    externalSensorData[pipeNum - 1].humidityMin = min(externalSensorData[pipeNum - 1].humidityMin, externalSensorData[pipeNum - 1].humidity);
                }

                externalSensorData[pipeNum - 1].humidityMax = max(externalSensorData[pipeNum - 1].humidityMax, externalSensorData[pipeNum - 1].humidity);
                externalSensorData[pipeNum - 1].temperature = float(externalSensor.temperature) / 100;

                if (externalSensorData[pipeNum - 1].temperatureMin == NULL)
                {
                    externalSensorData[pipeNum - 1].temperatureMin = externalSensorData[pipeNum - 1].temperature;
                }
                else
                {
                    externalSensorData[pipeNum - 1].temperatureMin = min(externalSensorData[pipeNum - 1].temperatureMin, externalSensorData[pipeNum - 1].temperature);
                }

                externalSensorData[pipeNum - 1].temperatureMax = max(externalSensorData[pipeNum - 1].temperatureMax, externalSensorData[pipeNum - 1].temperature);
                externalSensorData[pipeNum - 1].dewPoint = dew_point(externalSensorData[pipeNum - 1].temperature, externalSensorData[pipeNum - 1].humidity);
                externalSensorData[pipeNum - 1].humIndex = hum_index(externalSensorData[pipeNum - 1].temperature, externalSensorData[pipeNum - 1].dewPoint);

                xSemaphoreGive(xGlobalVariablesMutex);
            } else {
                ESP_LOGE(TAG, "Could not obtain the semaphore xGlobalVariablesMutex from task %s", pcTaskGetTaskName(NULL));
            }            

            ESP_LOGI(
                TAG,
                "Sensor %i:\nTemp: %02.2f\nHum: %02.2f\nDew: %i\nHI: %i\n",
                pipeNum,
                externalSensorData[pipeNum - 1].temperature,
                externalSensorData[pipeNum - 1].humidity,
                externalSensorData[pipeNum - 1].dewPoint,
                externalSensorData[pipeNum - 1].humIndex);

            if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastExternalTemperatureHistoryUpdateTime > INTERVAL_15_MIN)
            {
                lastExternalTemperatureHistoryUpdateTime = (xTaskGetTickCount() * portTICK_PERIOD_MS);

                for (int n = 0; n < CONFIG_APP_RF_SENSORS_COUNT; n++)
                {
                    for (int i = 0; i < 95; i++)
                    {
                        externalTemperatureLast24H[n][i] = externalTemperatureLast24H[n][i + 1];
                    }

                    externalTemperatureLast24H[n][95] = externalSensorData[n].temperature;
                }
            }

            if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastExternalTemperatureHistoryOneHourUpdateTime > INTERVAL_1_HOUR)
            {
                lastExternalTemperatureHistoryOneHourUpdateTime = (xTaskGetTickCount() * portTICK_PERIOD_MS);

                for (int n = 0; n < CONFIG_APP_RF_SENSORS_COUNT; n++)
                {
                    for (int i = 0; i < 59; i++)
                    {
                        externalTemperatureLastHour[n][i] = externalTemperatureLastHour[n][i + 1];
                    }

                    externalTemperatureLastHour[n][59] = externalSensorData[n].temperature;
                }
            }

            if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastExternalHumidityHistoryUpdateTime > INTERVAL_15_MIN)
            {
                lastExternalHumidityHistoryUpdateTime = (xTaskGetTickCount() * portTICK_PERIOD_MS);

                for (int n = 0; n < CONFIG_APP_RF_SENSORS_COUNT; n++)
                {
                    for (int i = 0; i < 95; i++)
                    {
                        externalHumidityLast24H[n][i] = externalHumidityLast24H[n][i + 1];
                    }

                    externalHumidityLast24H[n][95] = externalSensorData[n].humidity;
                }
            }

            if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastExternalHumidityHistoryOneHourUpdateTime > INTERVAL_1_HOUR)
            {
                lastExternalHumidityHistoryOneHourUpdateTime = (xTaskGetTickCount() * portTICK_PERIOD_MS);

                for (int n = 0; n < CONFIG_APP_RF_SENSORS_COUNT; n++)
                {
                    for (int i = 0; i < 59; i++)
                    {
                        externalHumidityLastHour[n][i] = externalHumidityLastHour[n][i + 1];
                    }

                    externalHumidityLastHour[n][59] = externalSensorData[n].humidity;
                }
            }
        }
    }
}
