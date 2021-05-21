#ifndef UTILITIES_METEO_H
#define UTILITIES_METEO_H

#include "app-globals.h"
#include "app-time.h"
#include <stdio.h>
#include "math_util.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>

enum Trend
{
  T_RISING,
  T_STEADY,
  T_FALLING,
};

float dew_point(float temp, float humidity);
long hum_index(float temp, float dewPoint);
Trend getTrend(float *values, uint8_t startIndex, uint8_t size);
char getZambrettiChar(float P, Trend trend);
uint8_t getForecastImageNumberFromZambrettiChar(char zambrettiChar);
uint8_t getForecastImageNumber();
uint32_t getCO2Color(int co2);

#endif