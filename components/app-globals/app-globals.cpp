#include "app-globals.h"

SemaphoreHandle_t xGlobalVariablesMutex = NULL;

InternalSensorData internalSensorData;
InternalSensorData prevInternalSensorData;
ExternalSensor externalSensor;
ExternalSensorData externalSensorData[CONFIG_APP_RF_SENSORS_COUNT];
