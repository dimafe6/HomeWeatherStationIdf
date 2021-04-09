#ifndef UTILITIES_MONITORING_H
#define UTILITIES_MONITORING_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

float GetTaskHighWaterMarkPercent(TaskHandle_t task_handle, uint32_t stack_allotment);

#endif