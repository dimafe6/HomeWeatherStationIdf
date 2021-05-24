#ifndef APP_RF_H_
#define APP_RF_H_

#include "RF24.h"
#include "app-sensors.h"
#include "meteo_util.h"
#include "esp_log.h"
#include "app-display.h"
#include "app-mqtt.h"

void nrf24_task(void *pvParameters);

#endif