#ifndef APP_DS3231_H_
#define APP_DS3231_H_

#define BUILD_YEAR (__DATE__ + 7)

#include <time.h>
#include <sys/time.h>
#include <cstring>
#include "ds3231.h"
#include "esp_sntp.h"
#include "esp_log.h"

void set_timezone(const char *tz);
bool obtain_time_from_rtc();
void obtain_time_from_ntp();
struct tm *get_local_time();

#endif
