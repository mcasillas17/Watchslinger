#ifndef WATCHSLINGER_OPEN_WEATHER_MAP_PROVIDER_H
#define WATCHSLINGER_OPEN_WEATHER_MAP_PROVIDER_H

#include "WeatherProvider.h"

class OpenWeatherMapProvider : public WeatherProvider {
public:
  bool getWeather(const watchySettings &settings, weatherData &weather,
                  WeatherProviderResponse *response = nullptr) override;

private:
  String buildUrl(const watchySettings &settings) const;
  bool parseWeather(const String &payload, const watchySettings &settings,
                    weatherData &weather,
                    WeatherProviderResponse *response) const;
};

#endif
