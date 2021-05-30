#include "app-sensors.h"

unsigned long lastPressureHistoryUpdateTime = INTERVAL_1_HOUR;
unsigned long lastTemperatureHistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastHumidityHistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastCo2HistoryUpdateTime = INTERVAL_15_MIN;
unsigned long lastTemperatureHistoryOneHourUpdateTime = INTERVAL_1_HOUR;
unsigned long lastHumidityHistoryOneHourUpdateTime = INTERVAL_1_HOUR;
unsigned long lastCo2HistoryOneHourUpdateTime = INTERVAL_1_HOUR;

void bme280_task(void *pvParameters)
{
  const char *TAG = "BME280";

  bmp280_params_t params;
  bmp280_init_default_params(&params);
  bmp280_t dev;
  memset(&dev, 0, sizeof(bmp280_t));

  ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, (gpio_num_t)CONFIG_BME280_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BME280_I2C_SCL_GPIO));
  if (bmp280_init(&dev, &params) != ESP_OK)
  {
    bmp280_free_desc(&dev);
    vTaskDelete(NULL);

    return;
  }

  ESP_LOGI(TAG, "BMP280: found %s\n", dev.id == BME280_CHIP_ID ? "BME280" : "BMP280");

  float pressure, temperature, humidity, dewPoint;
  long humindex;

  while (1)
  {
    bmp280_force_measurement(&dev);
    bool busy = false;
    do
    {
      vTaskDelay(pdMS_TO_TICKS(1));
      bmp280_is_measuring(&dev, &busy);
    } while (busy);

    if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
    {
      ESP_LOGE(TAG, "Temperature/pressure reading failed!\n");

      continue;
    }

    dewPoint = dew_point(temperature, humidity);
    humindex = hum_index(temperature, dewPoint);

    if (xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY) == pdTRUE)
    {
      internalSensorData.temperature = temperature;

      if (internalSensorData.temperatureMin == NULL)
      {
        internalSensorData.temperatureMin = internalSensorData.temperature;
      }
      else
      {
        internalSensorData.temperatureMin = min(internalSensorData.temperatureMin, internalSensorData.temperature);
      }

      internalSensorData.temperatureMax = max(internalSensorData.temperatureMax, internalSensorData.temperature);
      internalSensorData.humidity = humidity;

      if (internalSensorData.humidityMin == NULL)
      {
        internalSensorData.humidityMin = internalSensorData.humidity;
      }
      else
      {
        internalSensorData.humidityMin = min(internalSensorData.humidityMin, internalSensorData.humidity);
      }

      internalSensorData.humidityMax = max(internalSensorData.humidityMax, internalSensorData.humidity);
      internalSensorData.pressure = pressure / 100.0F; //hPa
      internalSensorData.pressureMmHg = internalSensorData.pressure / 1.33322387415F;
      internalSensorData.dewPoint = (dewPoint * 100) / 100;
      internalSensorData.humIndex = humindex;

      xSemaphoreGive(xGlobalVariablesMutex);
    }
    else
    {
      ESP_LOGE(TAG, "Could not obtain the semaphore xGlobalVariablesMutex from task %s", pcTaskGetTaskName(NULL));
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastPressureHistoryUpdateTime > INTERVAL_1_HOUR)
    {
      lastPressureHistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 23; i++)
      {
        pressureLast24H[i] = pressureLast24H[i + 1];
        pressureLast24HmmHg[i] = pressureLast24HmmHg[i + 1];
      }

      pressureLast24H[23] = internalSensorData.pressure;
      pressureLast24HmmHg[23] = internalSensorData.pressureMmHg;
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastTemperatureHistoryUpdateTime > INTERVAL_15_MIN)
    {
      lastTemperatureHistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 95; i++)
      {
        temperatureLast24H[i] = temperatureLast24H[i + 1];
      }
      temperatureLast24H[95] = internalSensorData.temperature;
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastTemperatureHistoryOneHourUpdateTime > INTERVAL_1_HOUR)
    {
      lastTemperatureHistoryOneHourUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 59; i++)
      {
        temperatureLastHour[i] = temperatureLastHour[i + 1];
      }

      temperatureLastHour[59] = internalSensorData.temperature;
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastHumidityHistoryUpdateTime > INTERVAL_15_MIN)
    {
      lastHumidityHistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 95; i++)
      {
        humidityLast24H[i] = humidityLast24H[i + 1];
      }
      humidityLast24H[95] = internalSensorData.humidity;
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastHumidityHistoryOneHourUpdateTime > INTERVAL_1_HOUR)
    {
      lastHumidityHistoryOneHourUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 59; i++)
      {
        humidityLastHour[i] = humidityLastHour[i + 1];
      }
      humidityLastHour[59] = internalSensorData.humidity;
    }

    vTaskDelay(CONFIG_APP_BME280_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void mhz19_task(void *pvParameters)
{
  const char *TAG = "MHZ19";

  mhz19_init();

  while (1)
  {
    int co2 = mhz19_get_co2();

    if (xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY) == pdTRUE)
    {
      internalSensorData.co2 = co2;

      xSemaphoreGive(xGlobalVariablesMutex);
    }
    else
    {
      ESP_LOGE(TAG, "Could not obtain the semaphore xGlobalVariablesMutex from task %s", pcTaskGetTaskName(NULL));
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastCo2HistoryUpdateTime > INTERVAL_15_MIN)
    {
      lastCo2HistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 95; i++)
      {
        co2Last24H[i] = co2Last24H[i + 1];
      }
      co2Last24H[95] = (float)internalSensorData.co2;
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastCo2HistoryOneHourUpdateTime > INTERVAL_1_HOUR)
    {
      lastCo2HistoryOneHourUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      for (int i = 0; i < 59; i++)
      {
        co2LastHour[i] = co2LastHour[i + 1];
      }
      co2LastHour[59] = (float)internalSensorData.co2;
    }

    vTaskDelay(CONFIG_APP_MHZ19_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void bh1750_task(void *pvParameters)
{
  const char *TAG = "BH1750";

  i2c_dev_t dev;
  memset(&dev, 0, sizeof(i2c_dev_t));

  ESP_ERROR_CHECK(bh1750_init_desc(&dev, BH1750_ADDR_LO, 0, (gpio_num_t)CONFIG_BH1750_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BH1750_I2C_SCL_GPIO));

  if (bh1750_setup(&dev, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH) != ESP_OK)
  {
    ESP_LOGE(TAG, "BH1750 not found!\n");
    bh1750_free_desc(&dev);
    vTaskDelete(NULL);

    return;
  }

  while (1)
  {
    uint16_t lux;

    if (bh1750_read(&dev, &lux) != ESP_OK)
    {
      ESP_LOGE(TAG, "Could not read lux data!\n");
    }

    if (xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY) == pdTRUE)
    {
      internalSensorData.lux = lux;

      xSemaphoreGive(xGlobalVariablesMutex);
    }
    else
    {
      ESP_LOGE(TAG, "Could not obtain the semaphore xGlobalVariablesMutex from task %s", pcTaskGetTaskName(NULL));
    }
    vTaskDelay(CONFIG_APP_BH1750_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}