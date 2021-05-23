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

char getZambrettiChar(float P, Trend trend)
{
    const char *TAG = "Meteo";
    uint8_t M = get_local_time()->tm_mon + 1;
    char result = '0';
    ESP_LOGW(TAG, "Month: %i", M);

    // FALLING
    if (trend == T_FALLING)
    {
        double Z = 130 - (P / 8.1);
        // A Winter falling generally results in a Z value higher by 1 unit.
        if (M < 4 || M > 9)
        {
            Z += 1;
        }
        else
        {
            Z += 2;
        }

        Z = round(Z);
        switch (int(Z))
        {
        case 1:
            result = 'A';
            break; //Settled Fine
        case 2:
            result = 'B';
            break; //Fine Weather
        case 3:
            result = 'D';
            break; //Fine Becoming Less Settled
        case 4:
            result = 'H';
            break; //Fairly Fine Showers Later
        case 5:
            result = 'O';
            break; //Showery Becoming unsettled
        case 6:
            result = 'R';
            break; //Unsettled, Rain later
        case 7:
            result = 'U';
            break; //Rain at times, worse later
        case 8:
            result = 'V';
            break; //Rain at times, becoming very unsettled
        case 9:
            result = 'X';
            break; //Very Unsettled, Rain
        }
    }
    else if (trend == T_STEADY)
    {
        double Z = round(147 - (5 * (P / 37.6)));
        switch (int(Z))
        {
        case 10:
            result = 'A';
            break; //Settled Fine
        case 11:
            result = 'B';
            break; //Fine Weather
        case 12:
            result = 'E';
            break; //Fine, Possibly showers
        case 13:
            result = 'K';
            break; //Fairly Fine, Showers likely
        case 14:
            result = 'N';
            break; //Showery Bright Intervals
        case 15:
            result = 'P';
            break; //Changeable some rain
        case 16:
            result = 'S';
            break; //Unsettled, rain at times
        case 17:
            result = 'W';
            break; //Rain at Frequent Intervals
        case 18:
            result = 'X';
            break; //Very Unsettled, Rain
        case 19:
            result = 'Z';
            break; //Stormy, much rain
        }
    }
    else if (trend == T_RISING)
    {
        double Z = 179 - (2 * (P / 12.9));
        //A Summer rising, improves the prospects
        if (M < 4 || M > 9)
        {
            Z -= 1;
        }
        else
        {
            Z -= 2;
        }

        Z = round(Z);

        switch (int(Z))
        {
        case 20:
            result = 'A';
            break; //Settled Fine
        case 21:
            result = 'B';
            break; //Fine Weather
        case 22:
            result = 'C';
            break; //Becoming Fine
        case 23:
            result = 'F';
            break; //Fairly Fine, Improving
        case 24:
            result = 'G';
            break; //Fairly Fine, Possibly showers, early
        case 25:
            result = 'I';
            break; //Showery Early, Improving
        case 26:
            result = 'J';
            break; //Changeable, Improving
        case 27:
            result = 'L';
            break; //Rather Unsettled Clearing Later
        case 28:
            result = 'M';
            break; //Unsettled, Probably Improving
        case 29:
            result = 'Q';
            break; //Unsettled, short fine Intervals
        case 30:
            result = 'T';
            break; //Very Unsettled, Finer at times
        case 31:
            result = 'Y';
            break; //Stormy, possibly improving
        case 32:
            result = 'Z';
            break; //Stormy, much rain
        }
    }

    return result;
}

uint8_t getForecastImageNumberFromZambrettiChar(char zambrettiChar)
{
    int M = get_local_time()->tm_mon + 1;

    switch (zambrettiChar)
    {
    case 'A':
    case 'B':
        return 1;
        break;
    case 'D':
    case 'H':
    case 'O':
    case 'E':
    case 'K':
    case 'N':
    case 'G':
    case 'I':
        //Winter
        if (M < 4 || M > 9)
        {
            return 3;
            break;
        }
        else
        {
            return 2;
            break;
        }
        break;
    case 'R':
    case 'U':
    case 'V':
    case 'X':
    case 'S':
    case 'W':
        //Winter
        if (M < 4 || M > 9)
        {
            return 6;
            break;
        }
        else
        {
            return 5;
            break;
        }
    case 'Z':
    case 'Y':
        return 7;
        break;
    case 'P':
    case 'J':
    case 'L':
    case 'M':
    case 'T':
        return 8;
        break;
    case 'C':
    case 'F':
    case 'Q':
        return 4;
        break;
    default:
        return 9;
        break;
        break;
    }
}

uint8_t getForecastImageNumber()
{
    // If no pressure for last 3h
    if (0 == pressureLast24H[21])
    {
        // Return N/A
        return getForecastImageNumberFromZambrettiChar(' ');
    }

    return getForecastImageNumberFromZambrettiChar(getZambrettiChar(internalSensorData.pressure, getTrend(pressureLast24H, 21, 3)));
}

uint32_t getCO2Color(int co2)
{
    if (co2 > 0 && co2 <= 600)
    {
        return 960;
    }
    else if (co2 > 600 && co2 <= 800)
    {
        return 4800;
    }
    else if (co2 > 800 && co2 <= 1000)
    {
        return 33895;
    }
    else if (co2 > 1000 && co2 <= 2000)
    {
        return 39622;
    }
    else if (co2 > 2000 && co2 <= 5000)
    {
        return 39235;
    }
    else if (co2 > 5000)
    {
        return 57376;
    }

    return 0;
}

uint32_t getHumindexColor(int humindex)
{
    if (humindex <= 19)
    {
        return 1024;
    }
    if (humindex >= 20 && humindex <= 29)
    {
        return 832;
    }
    else if (humindex >= 30 && humindex <= 39)
    {
        return 50500;
    }
    else if (humindex >= 40 && humindex <= 45)
    {
        return 45828;
    }
    else if (humindex > 45)
    {
        return 55554;
    }

    return 0;
}