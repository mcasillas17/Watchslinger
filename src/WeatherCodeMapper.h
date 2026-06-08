#ifndef WATCHSLINGER_WEATHER_CODE_MAPPER_H
#define WATCHSLINGER_WEATHER_CODE_MAPPER_H

#include <stdint.h>
#include <string.h>

struct WatchslingerWeatherCodeMapping {
  int16_t conditionCode;
  const char *description;
};

inline WatchslingerWeatherCodeMapping watchslingerMapOpenMeteoCode(int16_t code) {
  if (code == 0) {
    return {800, "Clear"};
  }
  if (code == 1 || code == 2 || code == 3) {
    return {801, "Clouds"};
  }
  if (code == 45 || code == 48) {
    return {741, "Fog"};
  }
  if ((code >= 51 && code <= 57)) {
    return {300, "Drizzle"};
  }
  if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) {
    return {500, "Rain"};
  }
  if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
    return {600, "Snow"};
  }
  if (code == 95 || code == 96 || code == 99) {
    return {200, "Thunderstorm"};
  }
  return {800, "Unknown"};
}

inline bool watchslingerIsHttpsUrl(const char *url) {
  return url != nullptr && strncmp(url, "https://", 8) == 0;
}

inline bool watchslingerIsMissingOpenWeatherMapKey(const char *apiKey) {
  return apiKey == nullptr || apiKey[0] == '\0' ||
         strcmp(apiKey, "YOUR_OPENWEATHERMAP_API_KEY") == 0 ||
         strcmp(apiKey, "REPLACE_WITH_YOUR_OPENWEATHERMAP_API_KEY") == 0;
}

#endif
