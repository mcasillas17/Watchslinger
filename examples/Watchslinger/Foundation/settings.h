#ifndef WATCHSLINGER_FOUNDATION_SETTINGS_H
#define WATCHSLINGER_FOUNDATION_SETTINGS_H

#define WATCHSLINGER_WEATHER_LAT "40.7128"
#define WATCHSLINGER_WEATHER_LON "-74.0060"

// Uncomment this to compile the example with OpenWeatherMap instead of Open-Meteo.
// #define WATCHSLINGER_USE_OPENWEATHERMAP

#define WATCHSLINGER_OPENWEATHERMAP_API_KEY "YOUR_OPENWEATHERMAP_API_KEY"
#define WATCHSLINGER_OPENWEATHERMAP_KEY_IS_SENTINEL

#if defined(WATCHSLINGER_USE_OPENWEATHERMAP) &&                            \
    defined(WATCHSLINGER_OPENWEATHERMAP_KEY_IS_SENTINEL)
#error "Set WATCHSLINGER_OPENWEATHERMAP_API_KEY to your own key before selecting OpenWeatherMap."
#endif

watchySettings settings{
    .cityID = "",
    .lat = WATCHSLINGER_WEATHER_LAT,
    .lon = WATCHSLINGER_WEATHER_LON,
#ifdef WATCHSLINGER_USE_OPENWEATHERMAP
    .weatherAPIKey = WATCHSLINGER_OPENWEATHERMAP_API_KEY,
    .weatherURL =
        "https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}"
        "&lang={lang}&units={units}&appid={apiKey}",
#else
    .weatherAPIKey = "",
    .weatherURL = "",
#endif
    .weatherUnit = "metric",
    .weatherLang = "en",
    .weatherUpdateInterval = 30,
    .ntpServer = "pool.ntp.org",
    .gmtOffset = 0,
    .vibrateOClock = true,
};

#endif
