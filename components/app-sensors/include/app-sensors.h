#ifndef APP_SENSORS_H_
#define APP_SENSORS_H_

#include "app-globals.h"
#include "app-mqtt.h"
#include <BME280.h>
#include "MH-Z19.h"
#include "bh1750.h"
#include "meteo_util.h"
#include <string.h>
#include <stdio.h>

void bme280_task(void *pvParameters);
void mhz19_task(void *pvParameters);
void bh1750_task(void *pvParameters);

#endif
