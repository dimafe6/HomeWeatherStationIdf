#include "meteo_util.h"

float dew_point(float temp, float humidity)
{
  if (humidity == 0)
    return temp;

  double dew_numer = 243.04 * (log(double(humidity) / 100.0) + ((17.625 * temp) / (temp + 243.04)));
  double dew_denom = 17.625 - log(double(humidity) / 100.0) - ((17.625 * temp) / (temp + 243.04));

  if (dew_numer == 0)
    dew_numer = 1;

  return dew_numer / dew_denom;
}

long hum_index(float temp, float dewPoint)
{
  return round(temp + 0.5555 * (6.11 * exp(5417.753 * (1 / 273.16 - 1 / (273.15 + dewPoint))) - 10));
}
