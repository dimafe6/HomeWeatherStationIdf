#ifndef APP_WIFI_H_
#define APP_WIFI_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"

void wifi_init(void);
void wifi_scan(void);
void wifi_init_sta(const char *ssid = NULL, const char *password = NULL);

#endif