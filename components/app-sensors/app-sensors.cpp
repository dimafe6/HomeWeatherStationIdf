#include "app-sensors.h"

unsigned long lastPressureHistoryUpdateTime = CONFIG_PRESSURE_HISTORY_INTERVAL;
unsigned long lastTemperatureHistoryUpdateTime = CONFIG_TEMPERATURE_HISTORY_INTERVAL;
unsigned long lastHumidityHistoryUpdateTime = CONFIG_HUMIDITY_HISTORY_INTERVAL;
unsigned long lastCo2HistoryUpdateTime = CONFIG_CO2_HISTORY_INTERVAL;
unsigned long lastTemperatureHistoryOneHourUpdateTime = CONFIG_TEMPERATURE_HISTORY_ONE_HOUR_INTERVAL;
unsigned long lastHumidityHistoryOneHourUpdateTime = CONFIG_HUMIDITY_HISTORY_ONE_HOUR_INTERVAL;
unsigned long lastCo2HistoryOneHourUpdateTime = CONFIG_CO2_HISTORY_ONE_HOUR_INTERVAL;

void bme280_task(void *pvParameters)
{
  const char *TAG = "BME280";

  bmp280_params_t params;
  bmp280_init_default_params(&params);
  bmp280_t dev;
  memset(&dev, 0, sizeof(bmp280_t));

  ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, (gpio_num_t)CONFIG_BME280_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BME280_I2C_SCL_GPIO));
  ESP_ERROR_CHECK(bmp280_init(&dev, &params));

  ESP_LOGI(TAG, "BMP280: found %s\n", dev.id == BME280_CHIP_ID ? "BME280" : "BMP280");

  float pressure, temperature, humidity, dewPoint;
  long humindex;

  while (1)
  {
    if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
    {
      ESP_LOGE(TAG, "Temperature/pressure reading failed!\n");

      continue;
    }

    dewPoint = dew_point(temperature, humidity);
    humindex = hum_index(temperature, dewPoint);

    xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);

    internalSensorData.temperature = temperature;
    if (internalSensorData.temperature < internalSensorData.temperatureMin)
    {
      internalSensorData.temperatureMin = internalSensorData.temperature;
    }
    if (internalSensorData.temperature > internalSensorData.temperatureMax)
    {
      internalSensorData.temperatureMax = internalSensorData.temperature;
    }

    internalSensorData.humidity = humidity;
    if (internalSensorData.humidity < internalSensorData.humidityMin)
    {
      internalSensorData.humidityMin = internalSensorData.humidity;
    }
    if (internalSensorData.humidity > internalSensorData.humidityMax)
    {
      internalSensorData.humidityMax = internalSensorData.humidity;
    }

    internalSensorData.pressure = pressure / 100.0F; //hPa
    internalSensorData.pressureMmHg = internalSensorData.pressure / 1.33322387415F;
    internalSensorData.dewPoint = (dewPoint * 100) / 100;
    internalSensorData.humIndex = humindex;

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastPressureHistoryUpdateTime > CONFIG_PRESSURE_HISTORY_INTERVAL)
    {
      lastPressureHistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updatePressureHistory();
    }

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastTemperatureHistoryUpdateTime > CONFIG_TEMPERATURE_HISTORY_INTERVAL)
    {
      lastTemperatureHistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updateTemperatureHistory();
    }

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastTemperatureHistoryOneHourUpdateTime > CONFIG_TEMPERATURE_HISTORY_ONE_HOUR_INTERVAL)
    {
      lastTemperatureHistoryOneHourUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updateTemperatureHistoryOneHour();
    }

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastHumidityHistoryUpdateTime > CONFIG_HUMIDITY_HISTORY_INTERVAL)
    {
      lastHumidityHistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updateHumidityHistory();
    }

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastHumidityHistoryOneHourUpdateTime > CONFIG_HUMIDITY_HISTORY_ONE_HOUR_INTERVAL)
    {
      lastHumidityHistoryOneHourUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updateHumidityHistoryOneHour();
    }

    xSemaphoreGive(xGlobalVariablesMutex);

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

    xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);

    internalSensorData.co2 = co2;

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastCo2HistoryUpdateTime > CONFIG_CO2_HISTORY_INTERVAL)
    {
      lastCo2HistoryUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updateCO2History();
    }

    if (xTaskGetTickCount() * portTICK_PERIOD_MS - lastCo2HistoryOneHourUpdateTime > CONFIG_CO2_HISTORY_ONE_HOUR_INTERVAL)
    {
      lastCo2HistoryOneHourUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

      updateCO2HistoryOneHour();
    }

    xSemaphoreGive(xGlobalVariablesMutex);

    vTaskDelay(CONFIG_APP_MHZ19_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void bh1750_task(void *pvParameters)
{
  const char *TAG = "BH1750";

  i2c_dev_t dev;
  memset(&dev, 0, sizeof(i2c_dev_t));

  ESP_ERROR_CHECK(bh1750_init_desc(&dev, BH1750_ADDR_LO, 0, (gpio_num_t)CONFIG_BH1750_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BH1750_I2C_SCL_GPIO));
  ESP_ERROR_CHECK(bh1750_setup(&dev, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH));

  while (1)
  {
    uint16_t lux;

    if (bh1750_read(&dev, &lux) != ESP_OK)
    {
      ESP_LOGE(TAG, "Could not read lux data!\n");
    }

    xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);

    internalSensorData.lux = lux;

    xSemaphoreGive(xGlobalVariablesMutex);

    vTaskDelay(CONFIG_APP_BH1750_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

float dew_point(float temp, float humidity)
{
  if (humidity == 0)
    return temp;

  double dew_numer = 243.04 * (log(double(humidity) / 100.0) + ((17.625 * temp) / (temp + 243.04)));
  double dew_denom = 17.625 - log(double(humidity) / 100.0) - ((17.625 * temp) / (temp + 243.04));

  if (dew_numer == 0)
    dew_numer = 1;

  return dew_numer / dew_denom;
}

long hum_index(float temp, float dewPoint)
{
  return round(temp + 0.5555 * (6.11 * exp(5417.753 * (1 / 273.16 - 1 / (273.15 + dewPoint))) - 10));
}

void updatePressureHistory()
{
  for (int i = 0; i < 23; i++)
  {
    pressureLast24H[i] = pressureLast24H[i + 1];
    pressureLast24HmmHg[i] = pressureLast24HmmHg[i + 1];
  }

  pressureLast24H[23] = internalSensorData.pressure;
  pressureLast24HmmHg[23] = internalSensorData.pressureMmHg;
}

void updateTemperatureHistory()
{
  for (int i = 0; i < 95; i++)
  {
    temperatureLast24H[i] = temperatureLast24H[i + 1];
  }
  temperatureLast24H[95] = internalSensorData.temperature;
}

void updateTemperatureHistoryOneHour()
{
  for (int i = 0; i < 59; i++)
  {
    temperatureLastHour[i] = temperatureLastHour[i + 1];
  }

  temperatureLastHour[59] = internalSensorData.temperature;
}

void updateHumidityHistory()
{
  for (int i = 0; i < 95; i++)
  {
    humidityLast24H[i] = humidityLast24H[i + 1];
  }
  humidityLast24H[95] = internalSensorData.humidity;
}

void updateHumidityHistoryOneHour()
{
  for (int i = 0; i < 59; i++)
  {
    humidityLastHour[i] = humidityLastHour[i + 1];
  }
  humidityLastHour[59] = internalSensorData.humidity;
}

void updateCO2History()
{
  for (int i = 0; i < 95; i++)
  {
    co2Last24H[i] = co2Last24H[i + 1];
  }
  co2Last24H[95] = (float)internalSensorData.co2;
}

void updateCO2HistoryOneHour()
{
  for (int i = 0; i < 59; i++)
  {
    co2LastHour[i] = co2LastHour[i + 1];
  }
  co2LastHour[59] = (float)internalSensorData.co2;
}