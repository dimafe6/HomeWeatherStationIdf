#include "app-display.h"

const char *TAG = "Nextion";

const nextion_display_t *display;
const nextion_text_t *iTemp;
const nextion_text_t *iTempFract;
char displayBuffer[CONFIG_NEXTION_MAX_MSG_LENGTH] = {0};

void nextion_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Nextion Event %x", event_id);

    if (event_id == NEXTION_EVENT_DISPLAY_READY)
    {
        ESP_LOGI(TAG, "Display Ready");
    }
    else if (event_id == NEXTION_EVENT_CURRENT_PAGE_NUMBER)
    {
        uint8_t page = *(uint8_t *)event_data;
        ESP_LOGI(TAG, "Page Event");
    }
    else if (event_id == NEXTION_EVENT_TOUCH)
    {
        nextion_touch_event_data_t *data = (nextion_touch_event_data_t *)event_data;
        ESP_LOGI(TAG, "Touch Event: Page->%d | Component ID->%d | Event->%d", data->page_id, data->component_id, data->event_type);
    }
}

void init_display()
{
    display = (const nextion_display_t *)nextion_display_init(NULL);
    nextion_register_event_handler(display, (nextion_event_t)ESP_EVENT_ANY_ID, nextion_event_handler, (void *)display);

    nextion_descriptor_t descriptor = {
        .page_id = 0,
        .component_id = 2,
        .name = "iTemp"};

    iTemp = nextion_text_init(display, &descriptor);

    descriptor.component_id = 3;
    descriptor.name = "iTempFract";

    iTempFract = nextion_text_init(display, &descriptor);
}

void update_display()
{
    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(internalSensorData.temperature));

    iTemp->set_text(iTemp, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, ".%d", fraction(internalSensorData.temperature));

    iTempFract->set_text(iTempFract, displayBuffer);
}

void display_task(void *pvParameters)
{
    init_display();

    while (1)
    {
        xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);
        update_display();
        xSemaphoreGive(xGlobalVariablesMutex);

        vTaskDelay(CONFIG_APP_DISPLAY_UPDATE_INTERVAL / portTICK_PERIOD_MS);
    }
}