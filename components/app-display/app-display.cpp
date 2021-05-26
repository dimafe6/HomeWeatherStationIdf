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

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && (event_id == WIFI_EVENT_STA_DISCONNECTED))
    {
        nextion_set_text(display, "wifiSignal", "0");
        nextion_set_pco(display, "wifiSignal", 57376);
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        wifi_ap_record_t info;
        if (esp_wifi_sta_get_ap_info(&info) == ESP_OK)
        {
            if (info.rssi <= -85)
            {
                nextion_set_text(display, "wifiSignal", "1");
            }
            else if (info.rssi > -85 && info.rssi <= -67)
            {
                nextion_set_text(display, "wifiSignal", "2");
            }
            else if (info.rssi > -67)
            {
                nextion_set_text(display, "wifiSignal", "3");
            }
        }

        nextion_set_pco(display, "wifiSignal", 65535);
    }
}

void init_display()
{
    nextion_system_variables_t system_variables = {.bkcmd = 0};
    display = (const nextion_display_t *)nextion_display_init(&system_variables);
    nextion_register_event_handler(display, (nextion_event_t)ESP_EVENT_ANY_ID, nextion_event_handler, (void *)display);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL));

    display->send_cmd(display, "page 0");
}

void display_task(void *pvParameters)
{
    init_display();

    while (1)
    {
        print_indoor_sensor();
        print_current_outdoor_sensor();
        print_time();

        vTaskDelay(CONFIG_APP_DISPLAY_UPDATE_INTERVAL / portTICK_PERIOD_MS);
    }
}

void print_current_outdoor_sensor()
{
    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%i", currentOutdoorSensorId + 1);
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
    nextion_set_pco(display, "oHumIndex", getHumindexColor((int)externalSensorData[currentOutdoorSensorId].humIndex));

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

void print_time()
{
    struct tm *local = get_local_time();
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", local->tm_hour);
    nextion_set_text(display, "hour", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", local->tm_min);
    nextion_set_text(display, "minute", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02u.%02u.%02u", local->tm_mday, local->tm_mon + 1, local->tm_year - 100);
    nextion_set_text(display, "date", displayBuffer);
}

void print_indoor_sensor()
{
    nextion_set_text(display, "iTempSign", internalSensorData.temperature < 0 ? "-" : " ");

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(internalSensorData.temperature));
    nextion_set_text(display, "iTemp", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, ".%d", fraction(internalSensorData.temperature));
    nextion_set_text(display, "iTempFract", displayBuffer);

    switch (getTrend(temperatureLastHour, 0, 60))
    {
    case T_RISING:
        nextion_set_text(display, "iTempTrend", "8");
        break;
    case T_FALLING:
        nextion_set_text(display, "iTempTrend", "9");
        break;
    default:
        nextion_set_text(display, "iTempTrend", ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)internalSensorData.temperatureMin);
    nextion_set_text(display, "iTempMin", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)internalSensorData.temperatureMax);
    nextion_set_text(display, "iTempMax", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)internalSensorData.dewPoint);
    nextion_set_text(display, "iDewPoint", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)internalSensorData.humIndex);
    nextion_set_text(display, "iHumIndex", displayBuffer);
    nextion_set_pco(display, "iHumIndex", getHumindexColor((int)internalSensorData.humIndex));

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(internalSensorData.humidity));
    nextion_set_text(display, "iHum", displayBuffer);

    switch (getTrend(humidityLastHour, 0, 60))
    {
    case T_RISING:
        nextion_set_text(display, "iHumTrend", "8");
        break;
    case T_FALLING:
        nextion_set_text(display, "iHumTrend", "9");
        break;
    default:
        nextion_set_text(display, "iHumTrend", ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(internalSensorData.humidityMin));
    nextion_set_text(display, "iHumMin", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(internalSensorData.humidityMax));
    nextion_set_text(display, "iHumMax", displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%i", internalSensorData.co2);
    nextion_set_text(display, "co2", displayBuffer);
    nextion_set_pco(display, "co2", getCO2Color(internalSensorData.co2));

    switch (getTrend(co2LastHour, 0, 3))
    {
    case T_RISING:
        nextion_set_text(display, "co2Trend", "8");
        break;
    case T_FALLING:
        nextion_set_text(display, "co2Trend", "9");
        break;
    default:
        nextion_set_text(display, "co2Trend", ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%i", internalSensorData.pressureMmHg);
    nextion_set_text(display, "pressure", displayBuffer);

    if (0 == pressureLast24H[21])
    {
        nextion_set_text(display, "pressureTrend", ":");
    }
    else
    {
        // Trend for last 3h
        switch (getTrend(pressureLast24H, 21, 3))
        {
        case T_RISING:
            nextion_set_text(display, "pressureTrend", "8");
            break;
        case T_FALLING:
            nextion_set_text(display, "pressureTrend", "9");
            break;
        default:
            nextion_set_text(display, "pressureTrend", ":");
            break;
        }
    }

    nextion_set_pic(display, "forecastImg", getForecastImageNumber());
}