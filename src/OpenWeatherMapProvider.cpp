#include "OpenWeatherMapProvider.h"

#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <TimeLib.h>
#include "WeatherCodeMapper.h"

namespace {
bool jsonHasType(const JSONVar &value, const char *type) {
  return JSON.typeof(value) == String(type);
}
}

String OpenWeatherMapProvider::buildUrl(const watchySettings &settings) const {
  if (watchslingerIsMissingOpenWeatherMapKey(settings.weatherAPIKey.c_str())) {
    return "";
  }

  String url = settings.weatherURL;
  if (url.length() == 0) {
    if (settings.cityID.length() > 0) {
      url =
          "https://api.openweathermap.org/data/2.5/weather?id={cityID}"
          "&lang={lang}&units={units}&appid={apiKey}";
    } else {
      url =
          "https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}"
          "&lang={lang}&units={units}&appid={apiKey}";
    }
  }

  if (!watchslingerIsHttpsUrl(url.c_str())) {
    return "";
  }

  url.replace("{cityID}", settings.cityID);
  url.replace("{lat}", settings.lat);
  url.replace("{lon}", settings.lon);
  url.replace("{units}", settings.weatherUnit);
  url.replace("{lang}", settings.weatherLang);
  url.replace("{apiKey}", settings.weatherAPIKey);
  return url;
}

bool OpenWeatherMapProvider::parseWeather(
    const String &payload, const watchySettings &settings, weatherData &weather,
    WeatherProviderResponse *providerResponse) const {
  JSONVar response = JSON.parse(payload);
  if (JSON.typeof(response) == "undefined") {
    return false;
  }
  if (!jsonHasType(response, "object")) {
    return false;
  }

  JSONVar main = response["main"];
  if (!jsonHasType(main, "object")) {
    return false;
  }
  JSONVar temperatureValue = main["temp"];
  if (!jsonHasType(temperatureValue, "number")) {
    return false;
  }

  JSONVar weatherArray = response["weather"];
  if (!jsonHasType(weatherArray, "array")) {
    return false;
  }
  if (weatherArray.length() < 1) {
    return false;
  }
  JSONVar weatherEntry = weatherArray[0];
  if (!jsonHasType(weatherEntry, "object")) {
    return false;
  }
  JSONVar conditionCodeValue = weatherEntry["id"];
  JSONVar descriptionValue = weatherEntry["main"];
  if (!jsonHasType(conditionCodeValue, "number") ||
      !jsonHasType(descriptionValue, "string")) {
    return false;
  }

  JSONVar sys = response["sys"];
  if (!jsonHasType(sys, "object")) {
    return false;
  }
  JSONVar sunriseValue = sys["sunrise"];
  JSONVar sunsetValue = sys["sunset"];
  JSONVar timezoneValue = response["timezone"];
  if (!jsonHasType(sunriseValue, "number") ||
      !jsonHasType(sunsetValue, "number") ||
      !jsonHasType(timezoneValue, "number")) {
    return false;
  }

  tmElements_t parsedSunrise;
  tmElements_t parsedSunset;
  breakTime(static_cast<time_t>(static_cast<int>(sunriseValue)),
            parsedSunrise);
  breakTime(static_cast<time_t>(static_cast<int>(sunsetValue)), parsedSunset);

  weather.temperature =
      static_cast<int8_t>(static_cast<int>(temperatureValue));
  weather.weatherConditionCode =
      static_cast<int16_t>(static_cast<int>(conditionCodeValue));
  weather.weatherDescription = static_cast<const char *>(descriptionValue);
  weather.isMetric = settings.weatherUnit == String("metric");
  weather.sunrise = parsedSunrise;
  weather.sunset = parsedSunset;
  weather.external = true;

  if (providerResponse != nullptr) {
    providerResponse->hasTimezoneOffset = true;
    providerResponse->timezoneOffset = static_cast<long>(
        static_cast<int>(timezoneValue));
  }

  return true;
}

bool OpenWeatherMapProvider::getWeather(const watchySettings &settings,
                                        weatherData &weather,
                                        WeatherProviderResponse *response) {
  String url = buildUrl(settings);
  if (url.length() == 0) {
    return false;
  }

  HTTPClient http;
  http.setConnectTimeout(3000);
  if (!http.begin(url.c_str())) {
    return false;
  }

  int httpResponseCode = http.GET();
  if (httpResponseCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  return parseWeather(payload, settings, weather, response);
}
