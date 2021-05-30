#include "app-rf.h"

unsigned long lastExternalTemperatureHistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastExternalHumidityHistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastExternalTemperatureHistoryOneHourUpdateTime = INTERVAL_1_HOUR;
unsigned long lastExternalHumidityHistoryOneHourUpdateTime = INTERVAL_1_HOUR;
unsigned long lastSensorSignalCheckTime = INTERVAL_5_SEC;
unsigned long lastDataSendToMQTT = INTERVAL_15_MIN;

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

                if (externalSensor.temperature / 100 < 60 && externalSensor.temperature / 100 > -60 &&
                    externalSensor.humidity / 100 < 100 && externalSensor.humidity / 100 > 0)
                {
                    prevExternalSensorData[pipeNum - 1] = externalSensorData[pipeNum - 1];

                    externalSensorData[pipeNum - 1].sensorId = pipeNum;
                    struct timeval tv_now;
                    gettimeofday(&tv_now, NULL);
                    externalSensorData[pipeNum - 1].measurementTime = (uint32_t)tv_now.tv_sec;
                    externalSensorData[pipeNum - 1].battery = externalSensor.battery;
                    externalSensorData[pipeNum - 1].signal = radio.testRPD();
                    externalSensorData[pipeNum - 1].humidity = float(externalSensor.humidity) / 100;

                    if (prevExternalSensorData[pipeNum - 1].measurementTime > 0)
                    {
                        externalSensorData[pipeNum - 1].sleepTime = externalSensorData[pipeNum - 1].measurementTime - prevExternalSensorData[pipeNum - 1].measurementTime;
                    }

                    if (externalSensorData[pipeNum - 1].humidityMin == NULL)
                    {
                        externalSensorData[pipeNum - 1].humidityMin = externalSensorData[pipeNum - 1].humidity;
                    }
                    else
                    {
                        externalSensorData[pipeNum - 1].humidityMin = min(externalSensorData[pipeNum - 1].humidity, externalSensorData[pipeNum - 1].humidityMin);
                    }

                    externalSensorData[pipeNum - 1].humidityMax = max(externalSensorData[pipeNum - 1].humidity, externalSensorData[pipeNum - 1].humidityMax);
                    externalSensorData[pipeNum - 1].temperature = float(externalSensor.temperature) / 100;

                    if (externalSensorData[pipeNum - 1].temperatureMin == NULL)
                    {
                        externalSensorData[pipeNum - 1].temperatureMin = externalSensorData[pipeNum - 1].temperature;
                    }
                    else
                    {
                        externalSensorData[pipeNum - 1].temperatureMin = min(externalSensorData[pipeNum - 1].temperature, externalSensorData[pipeNum - 1].temperatureMin);
                    }

                    externalSensorData[pipeNum - 1].temperatureMax = max(externalSensorData[pipeNum - 1].temperature, externalSensorData[pipeNum - 1].temperatureMax);
                    externalSensorData[pipeNum - 1].dewPoint = dew_point(externalSensorData[pipeNum - 1].temperature, externalSensorData[pipeNum - 1].humidity);
                    externalSensorData[pipeNum - 1].humIndex = hum_index(externalSensorData[pipeNum - 1].temperature, externalSensorData[pipeNum - 1].dewPoint);

                    ESP_LOGI(
                        TAG,
                        "Sensor %i:\nTemp: %02.2f\nHum: %02.2f\nDew: %i\nHI: %i\n",
                        pipeNum,
                        externalSensorData[pipeNum - 1].temperature,
                        externalSensorData[pipeNum - 1].humidity,
                        externalSensorData[pipeNum - 1].dewPoint,
                        externalSensorData[pipeNum - 1].humIndex);

                    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastDataSendToMQTT > INTERVAL_5_SEC)
                    {
                        lastDataSendToMQTT = xTaskGetTickCount() * portTICK_PERIOD_MS;

                        char buf[200] = {0};
                        char topic_name[10] = {0};

                        sprintf(topic_name, "outdoor/%i", pipeNum);
                        sprintf(
                            buf,
                            R"({"temp":%2.2f,"hum":%2.2f,"dp":%i,"hi":%i,"bat":%i})",
                            externalSensorData[pipeNum - 1].temperature,
                            externalSensorData[pipeNum - 1].humidity,
                            externalSensorData[pipeNum - 1].dewPoint,
                            externalSensorData[pipeNum - 1].humIndex,
                            externalSensorData[pipeNum - 1].battery);
                        mqtt_pub_sensor(topic_name, buf);

                        ESP_LOGI(TAG, "MQTT string: %s", buf);
                    }
                }
                else
                {
                    ESP_LOGI(
                        TAG,
                        "Wrong sensor %i data:\nTemp(x100): %i\nHum(x100): %i\n",
                        pipeNum,
                        externalSensor.temperature,
                        externalSensor.humidity);
                }

                xSemaphoreGive(xGlobalVariablesMutex);
            }
            else
            {
                ESP_LOGE(TAG, "Could not obtain the semaphore xGlobalVariablesMutex from task %s", pcTaskGetTaskName(NULL));
            }
        }

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

        if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastSensorSignalCheckTime > INTERVAL_5_SEC)
        {
            lastSensorSignalCheckTime = (xTaskGetTickCount() * portTICK_PERIOD_MS);

            for (uint8_t i = 0; i < CONFIG_APP_RF_SENSORS_COUNT; i++)
            {
                if (externalSensorData[i].sensorId == 255)
                {
                    continue;
                }

                if (externalSensorData[i].sleepTime > 0 && externalSensorData[i].measurementTime > 0)
                {
                    struct timeval tv_now;
                    gettimeofday(&tv_now, NULL);
                    uint32_t lastDataReceiveSeconds = (uint32_t)tv_now.tv_sec - externalSensorData[i].measurementTime;
                    uint32_t losedMessages = lastDataReceiveSeconds / externalSensorData[i].sleepTime;

                    if (losedMessages >= CONFIG_APP_RF_MAX_LOSSS_MESSAGES_BEFORE_LOSE_SIGNAL)
                    {
                        externalSensorData[i].temperature = 0;
                        externalSensorData[i].temperatureMin = NULL;
                        externalSensorData[i].temperatureMax = NULL;
                        externalSensorData[i].humidity = 0;
                        externalSensorData[i].humidityMin = NULL;
                        externalSensorData[i].humidityMax = NULL;
                        externalSensorData[i].dewPoint = 0;
                        externalSensorData[i].humIndex = 0;
                        externalSensorData[i].battery = 255;
                        externalSensorData[i].signal = 255;
                    }
                }
            }
        }
    }
}
