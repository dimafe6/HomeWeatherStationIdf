#ifndef __APP_MQTT_H__
#define __APP_MQTT_H__

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "app-globals.h"

void mqtt_app_init();
void mqtt_pub_sensor(const char *topic, const char *data);
#endif