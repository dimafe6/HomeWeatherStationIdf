#ifndef UTILITIES_METEO_H
#define UTILITIES_METEO_H

#include <stdio.h>
#include "math_util.h"
#include <math.h>

enum Trend
{
  T_RISING,
  T_STEADY,
  T_FALLING,
};

float dew_point(float temp, float humidity);
long hum_index(float temp, float dewPoint);
Trend getTrend(float *values, uint8_t startIndex, uint8_t size);

#endif