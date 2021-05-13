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

Trend getTrend(float *values, uint8_t startIndex, uint8_t size)
{
    int sumX2 = 0;
    float sumY = 0;
    float sumXY = 0;
    int trendValue = 0;

    float historyItems[size] = {0};
    for (uint8_t i = 0; i < size; i++)
    {
        historyItems[i] = values[startIndex + i];
    }

    int timeDots[size] = {0};

    if (size % 2 == 0)
    {
        timeDots[0] = ((size - 1) * -1);
        for (uint8_t n = 1; n < size; n++)
        {
            timeDots[n] = timeDots[n - 1] + 2;
        }
    }
    else
    {
        timeDots[0] = ((size - 1) / 2) * -1;
        for (uint8_t n = 1; n < size; n++)
        {
            timeDots[n] = timeDots[n - 1] + 1;
        }
    }

    for (uint8_t i = 0; i < size; i++)
    {
        sumX2 += timeDots[i] * timeDots[i];
        sumY += historyItems[i];
        sumXY += historyItems[i] * timeDots[i];
    }

    trendValue = (sumY / size + (sumXY / sumX2) * timeDots[size - 1]) - (sumY / size + (sumXY / sumX2) * timeDots[0]);

    if (trendValue > 0)
    {
        return T_RISING;
    }
    else if (trendValue == 0)
    {
        return T_STEADY;
    }
    else
    {
        return T_FALLING;
    }
}