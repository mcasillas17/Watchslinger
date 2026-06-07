#ifndef WATCHSLINGER_CANONICAL_SETTINGS_H
#define WATCHSLINGER_CANONICAL_SETTINGS_H

watchySettings settings{
    .cityID = "",
    .lat = "",
    .lon = "",
    .weatherAPIKey = "",
    .weatherURL = "",
    .weatherUnit = "metric",
    .weatherLang = "en",
    .weatherUpdateInterval = 30,
    .ntpServer = "pool.ntp.org",
    .gmtOffset = 0,
    .vibrateOClock = true,
};

#endif
