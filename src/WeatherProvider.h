#ifndef WATCHSLINGER_WEATHER_PROVIDER_H
#define WATCHSLINGER_WEATHER_PROVIDER_H

#include "WatchslingerTypes.h"

struct WeatherProviderResponse {
  bool hasTimezoneOffset;
  long timezoneOffset;

  WeatherProviderResponse() : hasTimezoneOffset(false), timezoneOffset(0) {}
};

class WeatherProvider {
public:
  virtual ~WeatherProvider() {}
  virtual bool getWeather(const watchySettings &settings, weatherData &weather,
                          WeatherProviderResponse *response = nullptr) = 0;
};

#endif
