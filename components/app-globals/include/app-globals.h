#ifndef APP_GLOBALS_H_
#define APP_GLOBALS_H_

#define INTERVAL_15_MIN 900000
#define INTERVAL_1_HOUR 3600000

#include <stdint.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

struct InternalSensorData
{
  int co2;
  float temperature;
  float temperatureMin;
  float temperatureMax;
  float humidity;
  float humidityMin;
  float humidityMax;
  float pressure; //hPa
  int pressureMmHg;
  float dewPoint;
  int16_t humIndex;
  int lux;
};

struct __attribute__((__packed__)) ExternalSensor
{
  int16_t temperature;
  int16_t humidity;
  uint8_t battery;
};

struct ExternalSensorData
{
  uint8_t sensorId = 255;
  float temperature = 0;
  float temperatureMin = NULL;
  float temperatureMax = NULL;
  float humidity = 0;
  float humidityMin = NULL;
  float humidityMax = NULL;
  int16_t dewPoint = 0;
  int16_t humIndex = 0;
  uint8_t battery = 255;
  uint8_t signal = 255;
};

extern SemaphoreHandle_t xGlobalVariablesMutex;

extern InternalSensorData internalSensorData;
extern InternalSensorData prevInternalSensorData;
extern ExternalSensor externalSensor;
extern ExternalSensorData externalSensorData[CONFIG_APP_RF_SENSORS_COUNT];
extern ExternalSensorData prevExternalSensorData[CONFIG_APP_RF_SENSORS_COUNT];

extern float externalTemperatureLast24H[CONFIG_APP_RF_SENSORS_COUNT][96];
extern float externalTemperatureLastHour[CONFIG_APP_RF_SENSORS_COUNT][60];
extern float externalHumidityLast24H[CONFIG_APP_RF_SENSORS_COUNT][96];
extern float externalHumidityLastHour[CONFIG_APP_RF_SENSORS_COUNT][60];

extern float pressureLast24H[24];
extern uint8_t pressureLast24HmmHg[24];
extern float temperatureLast24H[96];
extern float temperatureLastHour[60];
extern float humidityLast24H[96];
extern float humidityLastHour[60];
extern float co2Last24H[96];
extern float co2LastHour[60];

extern uint8_t currentOutdoorSensorId;
#endif