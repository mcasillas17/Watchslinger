#ifndef SETTINGS_H
#define SETTINGS_H

#define LAT "40.7128"
#define LON "-74.0060"
#define TEMP_UNIT "metric"
#define TEMP_LANG "en"
#define WEATHER_UPDATE_INTERVAL 30
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600 * -5

watchySettings settings{
    .cityID = "",
    .lat = LAT,
    .lon = LON,
    .weatherAPIKey = "",
    .weatherURL = "",
    .weatherUnit = TEMP_UNIT,
    .weatherLang = TEMP_LANG,
    .weatherUpdateInterval = WEATHER_UPDATE_INTERVAL,
    .ntpServer = NTP_SERVER,
    .gmtOffset = GMT_OFFSET_SEC,
    .vibrateOClock = true,
};

#endif
