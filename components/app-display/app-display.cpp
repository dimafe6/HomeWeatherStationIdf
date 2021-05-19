#include "app-display.h"

const char *TAG = "Display";

const nextion_display_t *display;
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

        // oHot1
        if (data->component_id == 61 && data->event_type == NEXTION_TOUCH_EVENT_PRESS)
        {
            if (currentOutdoorSensorId == 0)
            {
                currentOutdoorSensorId = 4;
            }
            else
            {
                currentOutdoorSensorId--;
            }

            print_current_outdoor_sensor();
        }

        // oHot2
        if (data->component_id == 62 && data->event_type == NEXTION_TOUCH_EVENT_PRESS)
        {
            currentOutdoorSensorId++;
            if (currentOutdoorSensorId >= CONFIG_APP_RF_SENSORS_COUNT)
            {
                currentOutdoorSensorId = 0;
            }

            print_current_outdoor_sensor();
        }

        ESP_LOGI(TAG, "Current sensor ID: %d", currentOutdoorSensorId);
    }
}

void init_display()
{
    nextion_system_variables_t system_variables = {.bkcmd = 0};
    display = (const nextion_display_t *)nextion_display_init(&system_variables);
    nextion_register_event_handler(display, (nextion_event_t)ESP_EVENT_ANY_ID, nextion_event_handler, (void *)display);
}

void display_task(void *pvParameters)
{
    init_display();

    while (1)
    {
        print_current_outdoor_sensor();

        vTaskDelay(CONFIG_APP_DISPLAY_UPDATE_INTERVAL / portTICK_PERIOD_MS);
    }
}

void print_current_outdoor_sensor()
{
    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%i", currentOutdoorSensorId);
    nextion_set_text(display, "oSensIdx", displayBuffer);

    switch (externalSensorData[currentOutdoorSensorId].signal)
    {
    case 1:
        nextion_set_text(display, "oSignal", "5"); // Full signal
        break;
    case 0:
        nextion_set_text(display, "oSignal", "3"); // Low signal
        break;
    default:
        nextion_set_text(display, "oSignal", "0"); // No signal
        break;
    }

    if (externalSensorData[currentOutdoorSensorId].battery == 255)
    {
        nextion_set_text(display, "oBatt", "0"); // No battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 76)
    {
        nextion_set_text(display, "oBatt", "5"); // Full battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 51 && externalSensorData[currentOutdoorSensorId].battery <= 75)
    {
        nextion_set_text(display, "oBatt", "4"); // Good battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 26 && externalSensorData[currentOutdoorSensorId].battery <= 50)
    {
        nextion_set_text(display, "oBatt", "3"); // Half battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 6 && externalSensorData[currentOutdoorSensorId].battery <= 25)
    {
        nextion_set_text(display, "oBatt", "2"); // Low battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 0 && externalSensorData[currentOutdoorSensorId].battery <= 5)
    {
        nextion_set_text(display, "oBatt", "1"); // Critical battery
    }

    nextion_set_text(display, "oTempSign", externalSensorData[currentOutdoorSensorId].temperature < 0 ? "-" : " ");

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].temperature));
    nextion_set_text(display, "oTemp", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, ".%d", fraction(externalSensorData[currentOutdoorSensorId].temperature));
    nextion_set_text(display, "oTempFract", displayBuffer);

    switch (getTrend(externalTemperatureLastHour[currentOutdoorSensorId], 0, 60))
    {
    case T_RISING:
        nextion_set_text(display, "oTempTrend", "8");
        break;
    case T_FALLING:
        nextion_set_text(display, "oTempTrend", "9");
        break;
    default:
        nextion_set_text(display, "oTempTrend", ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].temperatureMin);
    nextion_set_text(display, "oTempMin", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].temperatureMax);
    nextion_set_text(display, "oTempMax", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].dewPoint);
    nextion_set_text(display, "oDewPoint", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].humIndex);
    nextion_set_text(display, "oHumIndex", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].humidity));
    nextion_set_text(display, "oHum", displayBuffer);

    switch (getTrend(externalHumidityLastHour[currentOutdoorSensorId], 0, 60))
    {
    case T_RISING:
        nextion_set_text(display, "oHumTrend", "8");
        break;
    case T_FALLING:
        nextion_set_text(display, "oHumTrend", "9");
        break;
    default:
        nextion_set_text(display, "oHumTrend", ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].humidityMin));
    nextion_set_text(display, "oHumMin", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].humidityMax));
    nextion_set_text(display, "oHumMax", displayBuffer);
}