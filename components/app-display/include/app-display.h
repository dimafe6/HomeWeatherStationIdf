#ifndef APP_DISPLAY_H_
#define APP_DISPLAY_H_

#include <cstring>
#include "app-globals.h"
#include "nextion_display.h"
#include "nextion_object.h"
#include "esp_log.h"
#include "meteo_util.h"

void init_display();
void display_task(void *pvParameters);
void print_current_outdoor_sensor();

#endif