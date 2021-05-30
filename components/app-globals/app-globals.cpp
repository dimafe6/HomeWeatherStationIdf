#include "app-globals.h"

SemaphoreHandle_t xGlobalVariablesMutex = NULL;

InternalSensorData internalSensorData;
InternalSensorData prevInternalSensorData;
ExternalSensor externalSensor;
ExternalSensorData externalSensorData[CONFIG_APP_RF_SENSORS_COUNT];
ExternalSensorData prevExternalSensorData[CONFIG_APP_RF_SENSORS_COUNT];

float externalTemperatureLast24H[CONFIG_APP_RF_SENSORS_COUNT][96] = {0};
float externalTemperatureLastHour[CONFIG_APP_RF_SENSORS_COUNT][60] = {0};
float externalHumidityLast24H[CONFIG_APP_RF_SENSORS_COUNT][96] = {0};
float externalHumidityLastHour[CONFIG_APP_RF_SENSORS_COUNT][60] = {0};

float pressureLast24H[24] = {0};
uint8_t pressureLast24HmmHg[24] = {0};
float temperatureLast24H[96] = {0};
float temperatureLastHour[60] = {0};
float humidityLast24H[96] = {0};
float humidityLastHour[60] = {0};
float co2Last24H[96] = {0};
float co2LastHour[60] = {0};

uint8_t currentOutdoorSensorId = 0;

char mac_address[18] = {0};