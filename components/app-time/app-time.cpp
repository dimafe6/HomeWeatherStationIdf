#include "app-time.h"

static const char *TAG = "Time";

static void sync_rtc_time_from_local_time()
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, (gpio_num_t)CONFIG_BME280_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BME280_I2C_SCL_GPIO));

    time_t now = 0;
    time(&now);
    ds3231_set_time(&dev, localtime(&now));

    struct tm *local;
    local = localtime(&now);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
    ESP_ERROR_CHECK(ds3231_free_desc(&dev));
}

void time_sync_notification_cb(struct timeval *tv)
{
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
    {
        ESP_LOGI(TAG, "Time successfully synced from the NTP");
        sync_rtc_time_from_local_time();
    }
}

void set_timezone(const char *tz)
{
    setenv("TZ", tz, 1);
    tzset();
}

void obtain_time_from_ntp()
{
    ESP_LOGI(TAG, "Trying to get time from NTP");

    sntp_stop();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

bool obtain_time_from_rtc()
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, (gpio_num_t)CONFIG_BME280_I2C_SDA_GPIO, (gpio_num_t)CONFIG_BME280_I2C_SCL_GPIO));

    struct tm tim;
    if (ds3231_get_time(&dev, &tim) != ESP_OK)
    {
        return false;
    }

    time_t t = mktime(&tim);
    struct timeval nowtime = {.tv_sec = t};
    settimeofday(&nowtime, NULL);

    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d", tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec);
    ESP_ERROR_CHECK(ds3231_free_desc(&dev));

    return true;
}