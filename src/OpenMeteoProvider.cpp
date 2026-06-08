#include "OpenMeteoProvider.h"

#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <math.h>
#include <stdio.h>
#include "WeatherCodeMapper.h"

String OpenMeteoProvider::buildUrl(const watchySettings &settings) const {
  if (settings.lat.length() == 0 || settings.lon.length() == 0) {
    return "";
  }

  String temperatureUnit =
      settings.weatherUnit == String("imperial") ? "fahrenheit" : "celsius";
  String url =
      "https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}"
      "&current=temperature_2m,weather_code&daily=sunrise,sunset"
      "&timezone=auto&temperature_unit={temperatureUnit}";
  url.replace("{lat}", settings.lat);
  url.replace("{lon}", settings.lon);
  url.replace("{temperatureUnit}", temperatureUnit);
  return url;
}

bool OpenMeteoProvider::parseIsoMinute(const String &value,
                                       tmElements_t &time) const {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  if (sscanf(value.c_str(), "%4d-%2d-%2dT%2d:%2d", &year, &month, &day,
             &hour, &minute) != 5) {
    return false;
  }

  time.Year = CalendarYrToTm(year);
  time.Month = month;
  time.Day = day;
  time.Hour = hour;
  time.Minute = minute;
  time.Second = 0;
  return true;
}

bool OpenMeteoProvider::parseWeather(const String &payload,
                                     const watchySettings &settings,
                                     weatherData &weather) const {
  JSONVar response = JSON.parse(payload);
  if (JSON.typeof(response) == "undefined") {
    return false;
  }

  JSONVar current = response["current"];
  if (JSON.typeof(current) == "undefined") {
    return false;
  }

  double temperature = static_cast<double>(current["temperature_2m"]);
  int16_t weatherCode =
      static_cast<int16_t>(static_cast<int>(current["weather_code"]));
  WatchslingerWeatherCodeMapping mapped =
      watchslingerMapOpenMeteoCode(weatherCode);

  weather.temperature = static_cast<int8_t>(lround(temperature));
  weather.weatherConditionCode = mapped.conditionCode;
  weather.weatherDescription = mapped.description;
  weather.isMetric = settings.weatherUnit != String("imperial");
  weather.external = true;

  JSONVar daily = response["daily"];
  if (JSON.typeof(daily) != "undefined") {
    JSONVar sunrise = daily["sunrise"];
    JSONVar sunset = daily["sunset"];
    if (JSON.typeof(sunrise) != "undefined" &&
        JSON.typeof(sunset) != "undefined") {
      String sunriseValue = JSONVar::stringify(sunrise[0]);
      String sunsetValue = JSONVar::stringify(sunset[0]);
      sunriseValue.replace("\"", "");
      sunsetValue.replace("\"", "");
      parseIsoMinute(sunriseValue, weather.sunrise);
      parseIsoMinute(sunsetValue, weather.sunset);
    }
  }

  return true;
}

bool OpenMeteoProvider::getWeather(const watchySettings &settings,
                                   weatherData &weather,
                                   WeatherProviderResponse *response) {
  (void)response;
  String url = buildUrl(settings);
  if (!watchslingerIsHttpsUrl(url.c_str())) {
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
  return parseWeather(payload, settings, weather);
}
