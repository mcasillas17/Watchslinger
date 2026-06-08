#include "OpenWeatherMapProvider.h"

#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <TimeLib.h>
#include "WeatherCodeMapper.h"

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

  weather.temperature =
      static_cast<int8_t>(static_cast<int>(response["main"]["temp"]));
  weather.weatherConditionCode =
      static_cast<int16_t>(static_cast<int>(response["weather"][0]["id"]));
  String description = JSONVar::stringify(response["weather"][0]["main"]);
  description.replace("\"", "");
  weather.weatherDescription = description;
  weather.isMetric = settings.weatherUnit == String("metric");
  weather.external = true;

  breakTime(static_cast<time_t>(static_cast<int>(response["sys"]["sunrise"])),
            weather.sunrise);
  breakTime(static_cast<time_t>(static_cast<int>(response["sys"]["sunset"])),
            weather.sunset);

  if (providerResponse != nullptr) {
    providerResponse->hasTimezoneOffset = true;
    providerResponse->timezoneOffset =
        static_cast<long>(static_cast<int>(response["timezone"]));
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
