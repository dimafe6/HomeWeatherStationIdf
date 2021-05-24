#ifndef __APP_MQTT_H__
#define __APP_MQTT_H__

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"

void mqtt_app_init();
void mqtt_pub(const char *topic, const char *data, int len = 0, int qos = 2, int retain = 0);

#endif