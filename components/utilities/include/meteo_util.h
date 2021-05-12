#ifndef UTILITIES_METEO_H
#define UTILITIES_METEO_H

#include "math_util.h"
#include <math.h>

float dew_point(float temp, float humidity);
long hum_index(float temp, float dewPoint);

#endif