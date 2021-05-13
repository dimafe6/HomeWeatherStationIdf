#include "app-display.h"

const char *TAG = "Nextion";

const nextion_display_t *display;
const nextion_text_t *iTemp;
const nextion_text_t *iTempFract;
const nextion_text_t *oTemp;
const nextion_text_t *oTempFract;
const nextion_text_t *oTempSign;
const nextion_text_t *oTempTrend;
const nextion_text_t *oTempMin;
const nextion_text_t *oTempMax;
const nextion_text_t *oDewPoint;
const nextion_text_t *oHumIndex;
const nextion_text_t *oHum;
const nextion_text_t *oHumTrend;
const nextion_text_t *oHumMin;
const nextion_text_t *oHumMax;
const nextion_text_t *oSignal;
const nextion_text_t *oBatt;
const nextion_text_t *oSensIdx;

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
        if (data->component_id == 60 && data->event_type == NEXTION_TOUCH_EVENT_PRESS)
        {
            xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);
            if (currentOutdoorSensorId == 0)
            {
                currentOutdoorSensorId = 4;
            }
            else
            {
                currentOutdoorSensorId--;
            }
            xSemaphoreGive(xGlobalVariablesMutex);

            print_current_outdoor_sensor();
        }

        // oHot2
        if (data->component_id == 62 && data->event_type == NEXTION_TOUCH_EVENT_PRESS)
        {
            xSemaphoreTake(xGlobalVariablesMutex, portMAX_DELAY);
            currentOutdoorSensorId++;
            if (currentOutdoorSensorId >= CONFIG_APP_RF_SENSORS_COUNT)
            {
                currentOutdoorSensorId = 0;
            }
            xSemaphoreGive(xGlobalVariablesMutex);

            print_current_outdoor_sensor();
        }
    }
}

void init_display()
{
    nextion_system_variables_t system_variables = {
        .bkcmd = 0};

    display = (const nextion_display_t *)nextion_display_init(&system_variables);
    nextion_register_event_handler(display, (nextion_event_t)ESP_EVENT_ANY_ID, nextion_event_handler, (void *)display);

    nextion_descriptor_t descriptor = {
        .page_id = 0,
        .component_id = 2,
        .name = "iTemp"};

    iTemp = nextion_text_init(display, &descriptor);

    descriptor.component_id = 3;
    descriptor.name = "iTempFract";
    iTempFract = nextion_text_init(display, &descriptor);

    descriptor.component_id = 31;
    descriptor.name = "oTemp";
    oTemp = nextion_text_init(display, &descriptor);

    descriptor.component_id = 32;
    descriptor.name = "oTempFract";
    oTempFract = nextion_text_init(display, &descriptor);

    descriptor.component_id = 37;
    descriptor.name = "oTempSign";
    oTempSign = nextion_text_init(display, &descriptor);

    descriptor.component_id = 48;
    descriptor.name = "oTempTrend";
    oTempTrend = nextion_text_init(display, &descriptor);

    descriptor.component_id = 33;
    descriptor.name = "oTempMin";
    oTempMin = nextion_text_init(display, &descriptor);

    descriptor.component_id = 35;
    descriptor.name = "oTempMax";
    oTempMax = nextion_text_init(display, &descriptor);

    descriptor.component_id = 39;
    descriptor.name = "oDewPoint";
    oDewPoint = nextion_text_init(display, &descriptor);

    descriptor.component_id = 42;
    descriptor.name = "oHumIndex";
    oHumIndex = nextion_text_init(display, &descriptor);

    descriptor.component_id = 38;
    descriptor.name = "oHum";
    oHum = nextion_text_init(display, &descriptor);

    descriptor.component_id = 50;
    descriptor.name = "oHumTrend";
    oHumTrend = nextion_text_init(display, &descriptor);

    descriptor.component_id = 45;
    descriptor.name = "oHumMin";
    oHumMin = nextion_text_init(display, &descriptor);

    descriptor.component_id = 43;
    descriptor.name = "oHumMax";
    oHumMax = nextion_text_init(display, &descriptor);

    descriptor.component_id = 14;
    descriptor.name = "oSignal";
    oSignal = nextion_text_init(display, &descriptor);

    descriptor.component_id = 59;
    descriptor.name = "oBatt";
    oBatt = nextion_text_init(display, &descriptor);

    descriptor.component_id = 15;
    descriptor.name = "oSensIdx";
    oSensIdx = nextion_text_init(display, &descriptor);
}

void display_task(void *pvParameters)
{
    init_display();

    while (1)
    {
        vTaskDelay(CONFIG_APP_DISPLAY_UPDATE_INTERVAL / portTICK_PERIOD_MS);
    }
}

void print_current_outdoor_sensor()
{
    oTempSign->set_text(oTempSign, externalSensorData[currentOutdoorSensorId].temperature < 0 ? "-" : " ");

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].temperature));
    oTemp->set_text(oTemp, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, ".%d", fraction(externalSensorData[currentOutdoorSensorId].temperature));
    oTempFract->set_text(oTempFract, displayBuffer);

    switch (getTrend(externalTemperatureLastHour[currentOutdoorSensorId], 0, 60))
    {
    case T_RISING:
        oTempTrend->set_text(oTempTrend, "8");
        break;
    case T_FALLING:
        oTempTrend->set_text(oTempTrend, "9");
        break;
    default:
        oTempTrend->set_text(oTempTrend, ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].temperatureMin);
    oTempMin->set_text(oTempMin, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].temperatureMax);
    oTempMax->set_text(oTempMax, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].dewPoint);
    oDewPoint->set_text(oDewPoint, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)externalSensorData[currentOutdoorSensorId].humIndex);
    oHumIndex->set_text(oHumIndex, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].humidity));
    oHum->set_text(oHum, displayBuffer);

    switch (getTrend(externalHumidityLastHour[currentOutdoorSensorId], 0, 60))
    {
    case T_RISING:
        oHumTrend->set_text(oHumTrend, "8");
        break;
    case T_FALLING:
        oHumTrend->set_text(oHumTrend, "9");
        break;
    default:
        oHumTrend->set_text(oHumTrend, ":");
        break;
    }

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].humidityMin));
    oHumMin->set_text(oHumMin, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%02d", (int)abs(externalSensorData[currentOutdoorSensorId].humidityMax));
    oHumMax->set_text(oHumMax, displayBuffer);

    displayBuffer[0] = '\0';
    snprintf(displayBuffer, CONFIG_NEXTION_MAX_MSG_LENGTH, "%d", currentOutdoorSensorId + 1);
    oSensIdx->set_text(oSensIdx, displayBuffer);

    switch (externalSensorData[currentOutdoorSensorId].signal)
    {
    case 1:
        oSignal->set_text(oSignal, "5"); // Full signal
        break;
    case 0:
        oSignal->set_text(oSignal, "3"); // Low signal
        break;
    default:
        oSignal->set_text(oSignal, "0");             // No signal
        break;
    }

    if (externalSensorData[currentOutdoorSensorId].battery == 255)
    {
        oBatt->set_text(oBatt, "0"); // No battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 0 && externalSensorData[currentOutdoorSensorId].battery <= 5)
    {
        oBatt->set_text(oBatt, "1");             // Critical battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 6 && externalSensorData[currentOutdoorSensorId].battery <= 25)
    {
        oBatt->set_text(oBatt, "2"); // Low battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 26 && externalSensorData[currentOutdoorSensorId].battery <= 50)
    {
        oBatt->set_text(oBatt, "3"); // Half battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 51 && externalSensorData[currentOutdoorSensorId].battery <= 75)
    {
        oBatt->set_text(oBatt, "4"); // Good battery
    }
    else if (externalSensorData[currentOutdoorSensorId].battery >= 76)
    {
        oBatt->set_text(oBatt, "5"); // Full battery
    }
}