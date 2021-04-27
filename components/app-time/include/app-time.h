#ifndef APP_DS3231_H_
#define APP_DS3231_H_

#include <time.h>
#include <sys/time.h>
#include "ds3231.h"
#include "esp_sntp.h"
#include "esp_log.h"

void obtain_time(void);

#endif
