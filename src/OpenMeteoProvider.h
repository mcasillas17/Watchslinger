#ifndef WATCHSLINGER_OPEN_METEO_PROVIDER_H
#define WATCHSLINGER_OPEN_METEO_PROVIDER_H

#include "WeatherProvider.h"

class OpenMeteoProvider : public WeatherProvider {
public:
  bool getWeather(const watchySettings &settings, weatherData &weather,
                  WeatherProviderResponse *response = nullptr) override;

private:
  String buildUrl(const watchySettings &settings) const;
  bool parseWeather(const String &payload, const watchySettings &settings,
                    weatherData &weather) const;
  bool parseIsoMinute(const String &value, tmElements_t &time) const;
};

#endif
