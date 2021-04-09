#include "monitoring.h"

float GetTaskHighWaterMarkPercent(TaskHandle_t task_handle, uint32_t stack_allotment)
{
    UBaseType_t uxHighWaterMark;
    uint32_t diff;
    float result;

    uxHighWaterMark = uxTaskGetStackHighWaterMark(task_handle);

    diff = stack_allotment - uxHighWaterMark;

    result = ((float)diff / (float)stack_allotment) * 100.0;

    return result;
}