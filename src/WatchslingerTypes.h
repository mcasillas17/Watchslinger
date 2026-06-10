#ifndef WATCHSLINGER_TYPES_H
#define WATCHSLINGER_TYPES_H

#include <Arduino.h>
#include <TimeLib.h>

typedef struct weatherData {
  int8_t temperature;
  int16_t weatherConditionCode;
  bool isMetric;
  String weatherDescription;
  bool external;
  tmElements_t sunrise;
  tmElements_t sunset;
} weatherData;

typedef struct watchySettings {
  String cityID;
  String lat;
  String lon;
  String weatherAPIKey;
  String weatherURL;
  String weatherUnit;
  String weatherLang;
  int8_t weatherUpdateInterval;
  String ntpServer;
  int gmtOffset;
  bool vibrateOClock;
} watchySettings;

#endif
