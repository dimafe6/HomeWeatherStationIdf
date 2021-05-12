#include "app-sensors.h"

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
    if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
    {
      ESP_LOGE(TAG, "Temperature/pressure reading failed!\n");

      continue;
    }

    dewPoint = dew_point(temperature, humidity);
    humindex = hum_index(temperature, dewPoint);

    xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);

    internalSensorData.temperature = temperature;
    internalSensorData.temperatureMin = min(internalSensorData.temperatureMin, internalSensorData.temperature);
    internalSensorData.temperatureMax = max(internalSensorData.temperatureMax, internalSensorData.temperature);
    internalSensorData.humidity = humidity;
    internalSensorData.humidityMin = min(internalSensorData.humidityMin, internalSensorData.humidity);
    internalSensorData.humidityMax = max(internalSensorData.humidityMax, internalSensorData.humidity);
    internalSensorData.pressure = pressure / 100.0F; //hPa
    internalSensorData.pressureMmHg = internalSensorData.pressure / 1.33322387415F;
    internalSensorData.dewPoint = (dewPoint * 100) / 100;
    internalSensorData.humIndex = humindex;

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

    xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);

    internalSensorData.lux = lux;

    xSemaphoreGive(xGlobalVariablesMutex);

    vTaskDelay(CONFIG_APP_BH1750_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}