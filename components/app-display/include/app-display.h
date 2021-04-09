#ifndef APP_DISPLAY_H_
#define APP_DISPLAY_H_

#include "app-globals.h"
#include "nextion_display.h"
#include "nextion_text.h"
#include "esp_log.h"
#include "math.h"


void nextion_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void init_display();
void update_indoor_temperature(float temp);
void display_task(void *pvParameters);

#endif