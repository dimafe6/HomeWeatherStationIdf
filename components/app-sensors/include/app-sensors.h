#ifndef APP_SENSORS_H_
#define APP_SENSORS_H_

#include "app-globals.h"
#include <BME280.h>
#include "MH-Z19.h"
#include "bh1750.h"
#include "math.h"
#include <string.h>
#include <stdio.h>

void bme280_task(void *pvParameters);
void mhz19_task(void *pvParameters);
void bh1750_task(void *pvParameters);

float dew_point(float temp, float humidity);
long hum_index(float temp, float dewPoint);
void readAllSensors(bool force);
void updatePressureHistory();
void updateTemperatureHistory();
void updateHumidityHistory();
void updateTemperatureHistoryOneHour();
void updateHumidityHistoryOneHour();
void updateCO2History();
void updateCO2HistoryOneHour();

#endif
