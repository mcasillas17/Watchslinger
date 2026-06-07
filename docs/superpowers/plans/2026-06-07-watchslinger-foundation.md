# Watchslinger Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the Watchslinger foundation by adding first-class weather providers, finishing the menu/watchface foundation details, documenting customization, and making `pio run` build the Watchy v3 firmware.

**Architecture:** The current branch already has `Watchslinger`, `Watchy.h` compatibility, and registry-backed menu primitives. This plan keeps those layers, adds a small `WeatherProvider` interface with Open-Meteo default and OpenWeatherMap opt-in implementations, routes `getWeatherData()` through the selected provider, updates examples/docs, and validates with host tests plus `pio run`.

**Tech Stack:** Arduino-style C++11, arduino-esp32, PlatformIO, GxEPD2, Arduino_JSON, HTTPClient, existing Watchy RTC/BMA/WiFi code, host `g++` tests for framework-only helpers.

---

## Execution Context

- Worktree: `/Users/elopenmike/build/DIY/Watchy/Watchslinger/.worktrees/watchslinger-foundation-v2`
- Branch: `watchslinger-foundation-v2`
- Approved spec: `docs/superpowers/specs/2026-06-07-watchslinger-foundation-design.md`
- Preserve user work outside this worktree.
- Keep existing HAL behavior intact: display, RTC, accelerometer, vibration motor, battery ADC, WiFi, NTP, BLE, OTA, and sleep paths.

## File Structure

- Modify `platformio.ini`: make default `pio run` build the foundation example for Watchy v3 with required ESP32-S3, 8 MB flash, USB CDC, and partition settings.
- Create `src/WatchslingerTypes.h`: shared `weatherData` and `watchySettings` structs currently embedded in `Watchslinger.h`.
- Create `src/WeatherCodeMapper.h`: host-testable Open-Meteo code mapping, HTTPS check, and OpenWeatherMap API-key sentinel detection.
- Create `src/WeatherProvider.h`: provider interface and optional provider response metadata.
- Create `src/OpenMeteoProvider.h` and `src/OpenMeteoProvider.cpp`: default HTTPS no-key provider.
- Create `src/OpenWeatherMapProvider.h` and `src/OpenWeatherMapProvider.cpp`: HTTPS OpenWeatherMap compatibility provider requiring a user key.
- Modify `src/Watchslinger.h` and `src/Watchslinger.cpp`: add provider injection/defaulting, remove hardcoded `_getWeatherData()`, preserve fallback behavior.
- Modify `src/WatchslingerApp.h`: make registry counts and lookup skip invalid descriptors.
- Modify `test/host/test_app_registry.cpp`: cover invalid descriptor skipping.
- Create `test/host/test_weather_helpers.cpp`: cover mapping, HTTPS enforcement, and missing OpenWeatherMap key detection.
- Create `examples/Watchslinger/Foundation/Foundation.ino` and `examples/Watchslinger/Foundation/settings.h`: one minimal example showing custom watchface, two-entry custom menu, one stock screen wrapper, one custom app, and weather provider selection.
- Modify `examples/WatchFaces/*/settings.h`: remove the shared OpenWeatherMap API key and HTTP URL from checked-in examples.
- Modify `README.md`: document Watchslinger, compatibility, PlatformIO, watchfaces, menu customization, weather providers, and validation.

## Task 1: Complete The Watchy v3 PlatformIO Configuration

**Files:**
- Modify: `platformio.ini`

- [ ] **Step 1: Record the current missing configuration**

Run:

```bash
cd /Users/elopenmike/build/DIY/Watchy/Watchslinger/.worktrees/watchslinger-foundation-v2
grep -E 'src_dir|board_build.mcu|board_build.partitions|ARDUINO_USB_MODE|ARDUINO_USB_CDC_ON_BOOT|ARDUINO_WATCHY_V30' platformio.ini || true
```

Expected: output includes `-DARDUINO_WATCHY_V30`, but does not include `src_dir`, `board_build.mcu`, `board_build.partitions`, `ARDUINO_USB_MODE`, or `ARDUINO_USB_CDC_ON_BOOT`.

- [ ] **Step 2: Update `platformio.ini`**

Replace the file with:

```ini
[platformio]
default_envs = watchy_v3
src_dir = examples/Watchslinger/Canonical

[env:watchy_v3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.mcu = esp32s3
board_build.flash_size = 8MB
board_build.partitions = default_8MB.csv
board_upload.flash_size = 8MB
monitor_speed = 115200
lib_ldf_mode = deep+
lib_extra_dirs = .
build_flags =
  -DARDUINO_USB_MODE=1
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DARDUINO_WATCHY_V30
  -DCORE_DEBUG_LEVEL=0
  ; Rtc_Pcf8563 master uses WireBase; ESP32 Arduino exposes TwoWire.
  -DWireBase=TwoWire
lib_deps =
  adafruit/Adafruit GFX Library
  arduino-libraries/Arduino_JSON
  jchristensen/DS3232RTC
  arduino-libraries/NTPClient
  paulstoffregen/Time
  https://github.com/orbitalair/Rtc_Pcf8563.git#master
  https://github.com/ZinggJM/GxEPD2.git#master
  https://github.com/tzapu/WiFiManager.git#master
```

- [ ] **Step 3: Verify the required PlatformIO keys are present**

Run:

```bash
grep -E 'src_dir|board_build.mcu|board_build.partitions|ARDUINO_USB_MODE|ARDUINO_USB_CDC_ON_BOOT|ARDUINO_WATCHY_V30' platformio.ini
```

Expected: output contains `src_dir` plus the five Watchy v3 build settings.

- [ ] **Step 4: Commit**

Run:

```bash
git add platformio.ini
git commit -m "Configure PlatformIO for Watchslinger foundation" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 2: Add Host-Tested Weather Helpers And Shared Types

**Files:**
- Create: `src/WatchslingerTypes.h`
- Create: `src/WeatherCodeMapper.h`
- Create: `src/WeatherProvider.h`
- Create: `test/host/test_weather_helpers.cpp`
- Modify: `src/Watchslinger.h`

- [ ] **Step 1: Write the failing weather helper test**

Create `test/host/test_weather_helpers.cpp`:

```cpp
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/WeatherCodeMapper.h"

#define ASSERT_TRUE(condition)                                                   \
  do {                                                                          \
    if (!(condition)) {                                                         \
      printf("ASSERT_TRUE failed at line %d: %s\n", __LINE__, #condition);      \
      return 1;                                                                 \
    }                                                                           \
  } while (0)

#define ASSERT_EQ(expected, actual)                                              \
  do {                                                                          \
    if ((expected) != (actual)) {                                                \
      printf("ASSERT_EQ failed at line %d: expected %d got %d\n", __LINE__,     \
             static_cast<int>(expected), static_cast<int>(actual));             \
      return 1;                                                                 \
    }                                                                           \
  } while (0)

int main() {
  WatchslingerWeatherCodeMapping clear = watchslingerMapOpenMeteoCode(0);
  ASSERT_EQ(800, clear.conditionCode);
  ASSERT_TRUE(strcmp("Clear", clear.description) == 0);

  WatchslingerWeatherCodeMapping fog = watchslingerMapOpenMeteoCode(45);
  ASSERT_EQ(741, fog.conditionCode);
  ASSERT_TRUE(strcmp("Fog", fog.description) == 0);

  WatchslingerWeatherCodeMapping drizzle = watchslingerMapOpenMeteoCode(53);
  ASSERT_EQ(300, drizzle.conditionCode);
  ASSERT_TRUE(strcmp("Drizzle", drizzle.description) == 0);

  WatchslingerWeatherCodeMapping rain = watchslingerMapOpenMeteoCode(65);
  ASSERT_EQ(500, rain.conditionCode);
  ASSERT_TRUE(strcmp("Rain", rain.description) == 0);

  WatchslingerWeatherCodeMapping snow = watchslingerMapOpenMeteoCode(75);
  ASSERT_EQ(600, snow.conditionCode);
  ASSERT_TRUE(strcmp("Snow", snow.description) == 0);

  WatchslingerWeatherCodeMapping thunderstorm = watchslingerMapOpenMeteoCode(95);
  ASSERT_EQ(200, thunderstorm.conditionCode);
  ASSERT_TRUE(strcmp("Thunderstorm", thunderstorm.description) == 0);

  WatchslingerWeatherCodeMapping unknown = watchslingerMapOpenMeteoCode(999);
  ASSERT_EQ(800, unknown.conditionCode);
  ASSERT_TRUE(strcmp("Unknown", unknown.description) == 0);

  ASSERT_TRUE(watchslingerIsHttpsUrl("https://api.open-meteo.com/v1/forecast"));
  ASSERT_TRUE(!watchslingerIsHttpsUrl("http://api.openweathermap.org/data/2.5/weather"));
  ASSERT_TRUE(!watchslingerIsHttpsUrl(""));
  ASSERT_TRUE(!watchslingerIsHttpsUrl(nullptr));

  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey(""));
  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey(nullptr));
  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey("YOUR_OPENWEATHERMAP_API_KEY"));
  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey("REPLACE_WITH_YOUR_OPENWEATHERMAP_API_KEY"));
  ASSERT_TRUE(!watchslingerIsMissingOpenWeatherMapKey("user-supplied-key"));

  printf("all weather helper tests passed\n");
  return 0;
}
```

- [ ] **Step 2: Run the weather helper test to verify it fails**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_weather_helpers.cpp -o /tmp/watchslinger_weather_helpers_test
```

Expected: compile fails with `fatal error: '../../src/WeatherCodeMapper.h' file not found`.

- [ ] **Step 3: Add shared settings and weather types**

Create `src/WatchslingerTypes.h`:

```cpp
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
```

- [ ] **Step 4: Add weather helper functions**

Create `src/WeatherCodeMapper.h`:

```cpp
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
```

- [ ] **Step 5: Add the provider interface**

Create `src/WeatherProvider.h`:

```cpp
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
```

- [ ] **Step 6: Move shared structs out of `Watchslinger.h`**

In `src/Watchslinger.h`, add this include after `#include "config.h"`:

```cpp
#include "WatchslingerTypes.h"
```

Then remove the existing inline `typedef struct weatherData { ... } weatherData;` and `typedef struct watchySettings { ... } watchySettings;` blocks from `src/Watchslinger.h`.

- [ ] **Step 7: Run weather helper and existing host tests**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_weather_helpers.cpp -o /tmp/watchslinger_weather_helpers_test && /tmp/watchslinger_weather_helpers_test
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
g++ -std=c++11 -Isrc test/host/test_v3_macro.cpp -o /tmp/watchslinger_v3_macro_test && /tmp/watchslinger_v3_macro_test
g++ -std=c++11 -Isrc test/host/test_esp32s3_macro.cpp -o /tmp/watchslinger_esp32s3_macro_test && /tmp/watchslinger_esp32s3_macro_test
```

Expected: all commands pass and print the test success messages where present.

- [ ] **Step 8: Commit**

Run:

```bash
git add src/WatchslingerTypes.h src/WeatherCodeMapper.h src/WeatherProvider.h src/Watchslinger.h test/host/test_weather_helpers.cpp
git commit -m "Add weather provider primitives" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 3: Add Open-Meteo And OpenWeatherMap Providers

**Files:**
- Create: `src/OpenMeteoProvider.h`
- Create: `src/OpenMeteoProvider.cpp`
- Create: `src/OpenWeatherMapProvider.h`
- Create: `src/OpenWeatherMapProvider.cpp`

- [ ] **Step 1: Add the Open-Meteo provider header**

Create `src/OpenMeteoProvider.h`:

```cpp
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
```

- [ ] **Step 2: Add the Open-Meteo provider implementation**

Create `src/OpenMeteoProvider.cpp`:

```cpp
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
  int16_t weatherCode = static_cast<int16_t>(static_cast<int>(current["weather_code"]));
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
```

- [ ] **Step 3: Add the OpenWeatherMap provider header**

Create `src/OpenWeatherMapProvider.h`:

```cpp
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
                    weatherData &weather, WeatherProviderResponse *response) const;
};

#endif
```

- [ ] **Step 4: Add the OpenWeatherMap provider implementation**

Create `src/OpenWeatherMapProvider.cpp`:

```cpp
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

  weather.temperature = static_cast<int8_t>(static_cast<int>(response["main"]["temp"]));
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
```

- [ ] **Step 5: Run host tests**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_weather_helpers.cpp -o /tmp/watchslinger_weather_helpers_test && /tmp/watchslinger_weather_helpers_test
```

Expected: `all weather helper tests passed`.

- [ ] **Step 6: Commit**

Run:

```bash
git add src/OpenMeteoProvider.h src/OpenMeteoProvider.cpp src/OpenWeatherMapProvider.h src/OpenWeatherMapProvider.cpp
git commit -m "Add weather provider implementations" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 4: Route Watchslinger Weather Through The Selected Provider

**Files:**
- Modify: `src/Watchslinger.h`
- Modify: `src/Watchslinger.cpp`

- [ ] **Step 1: Update `Watchslinger.h` includes**

In `src/Watchslinger.h`, add:

```cpp
#include "WeatherProvider.h"
```

after:

```cpp
#include "WatchslingerTypes.h"
```

- [ ] **Step 2: Update `Watchslinger` constructors and fields**

Replace the current constructor block in `src/Watchslinger.h` with:

```cpp
  explicit Watchslinger(const watchySettings &s)
      : settings(s), customApps_(), menuMode_(WatchslingerMenuMode::StockOnly),
        activeApp_(nullptr), weatherProvider_(&defaultWeatherProvider()) {}
  explicit Watchslinger(const watchySettings &s, WeatherProvider &weatherProvider)
      : settings(s), customApps_(), menuMode_(WatchslingerMenuMode::StockOnly),
        activeApp_(nullptr), weatherProvider_(&weatherProvider) {}
  Watchslinger(const watchySettings &s, WatchslingerAppList customApps,
               WatchslingerMenuMode menuMode =
                   WatchslingerMenuMode::StockPlusCustom)
      : settings(s), customApps_(customApps), menuMode_(menuMode),
        activeApp_(nullptr), weatherProvider_(&defaultWeatherProvider()) {}
  Watchslinger(const watchySettings &s, WatchslingerAppList customApps,
               WatchslingerMenuMode menuMode, WeatherProvider &weatherProvider)
      : settings(s), customApps_(customApps), menuMode_(menuMode),
        activeApp_(nullptr), weatherProvider_(&weatherProvider) {}
```

Add this public declaration near `weatherData getWeatherData();`:

```cpp
  static WeatherProvider &defaultWeatherProvider();
```

Replace the private `_getWeatherData(...)` declaration with:

```cpp
  void _useInternalTemperatureWeather();
```

Add this private field after `WatchslingerApp *activeApp_;`:

```cpp
  WeatherProvider *weatherProvider_;
```

- [ ] **Step 3: Add the default provider include and instance**

In `src/Watchslinger.cpp`, add:

```cpp
#include "OpenMeteoProvider.h"
```

after the existing Watchslinger includes.

Inside the anonymous namespace, before the stock app instances, add:

```cpp
OpenMeteoProvider defaultWeatherProviderInstance;
```

After the anonymous namespace, add:

```cpp
WeatherProvider &Watchslinger::defaultWeatherProvider() {
  return defaultWeatherProviderInstance;
}
```

- [ ] **Step 4: Replace the weather implementation**

Replace `Watchslinger::getWeatherData()` and `Watchslinger::_getWeatherData(...)` in `src/Watchslinger.cpp` with:

```cpp
weatherData Watchslinger::getWeatherData() {
  currentWeather.isMetric = settings.weatherUnit == String("metric");
  if (weatherIntervalCounter < 0) {
    weatherIntervalCounter = settings.weatherUpdateInterval;
  }

  if (weatherIntervalCounter >= settings.weatherUpdateInterval) {
    bool fetchedExternalWeather = false;

    if (weatherProvider_ != nullptr && connectWiFi()) {
      WeatherProviderResponse providerResponse;
      fetchedExternalWeather =
          weatherProvider_->getWeather(settings, currentWeather, &providerResponse);

      if (fetchedExternalWeather && providerResponse.hasTimezoneOffset) {
        gmtOffset = providerResponse.timezoneOffset;
        syncNTP(gmtOffset);
      }

      WiFi.mode(WIFI_OFF);
      btStop();
    }

    if (!fetchedExternalWeather) {
      _useInternalTemperatureWeather();
    }

    weatherIntervalCounter = 0;
  } else {
    weatherIntervalCounter++;
  }

  return currentWeather;
}

void Watchslinger::_useInternalTemperatureWeather() {
  uint8_t temperature = sensor.readTemperature();
  if (!currentWeather.isMetric) {
    temperature = temperature * 9. / 5. + 32.;
  }
  currentWeather.temperature = temperature;
  currentWeather.weatherConditionCode = 800;
  currentWeather.weatherDescription = "Internal";
  currentWeather.external = false;
}
```

- [ ] **Step 5: Run host tests**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
g++ -std=c++11 -Isrc test/host/test_weather_helpers.cpp -o /tmp/watchslinger_weather_helpers_test && /tmp/watchslinger_weather_helpers_test
```

Expected: both tests pass.

- [ ] **Step 6: Commit**

Run:

```bash
git add src/Watchslinger.h src/Watchslinger.cpp
git commit -m "Route weather through provider interface" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 5: Skip Invalid Menu Descriptors

**Files:**
- Modify: `src/WatchslingerApp.h`
- Modify: `test/host/test_app_registry.cpp`

- [ ] **Step 1: Update the registry test for invalid descriptors**

Replace `test/host/test_app_registry.cpp` with:

```cpp
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/WatchslingerApp.h"

#define ASSERT_TRUE(condition)                                                   \
  do {                                                                          \
    if (!(condition)) {                                                         \
      printf("ASSERT_TRUE failed at line %d: %s\n", __LINE__, #condition);      \
      return 1;                                                                 \
    }                                                                           \
  } while (0)

#define ASSERT_EQ(expected, actual)                                              \
  do {                                                                          \
    if ((expected) != (actual)) {                                                \
      printf("ASSERT_EQ failed at line %d: expected %d got %d\n", __LINE__,     \
             static_cast<int>(expected), static_cast<int>(actual));             \
      return 1;                                                                 \
    }                                                                           \
  } while (0)

class Watchslinger {
public:
  Watchslinger() : legacyOpened(false) {}

  void openLegacy() { legacyOpened = true; }

  bool legacyOpened;
};

class DummyApp : public WatchslingerApp {
public:
  void onOpen(Watchslinger &) override {}
};

int main() {
  WatchslingerAppList emptyList;
  WatchslingerAppRegistry emptyRegistry(emptyList);
  ASSERT_EQ(0, emptyRegistry.count());
  ASSERT_TRUE(emptyRegistry.get(0) == nullptr);

  WatchslingerAppList invalidList(nullptr, 1);
  WatchslingerAppRegistry invalidRegistry(invalidList);
  ASSERT_EQ(0, invalidList.count);
  ASSERT_EQ(0, invalidRegistry.count());
  ASSERT_TRUE(invalidRegistry.get(0) == nullptr);

  DummyApp stockA;
  DummyApp stockB;
  DummyApp customA;

  const WatchslingerAppDescriptor stockItems[] = {
      {"About Watchslinger", &stockA},
      {nullptr, &stockA},
      {"Missing App", nullptr},
      {"Set Time", &stockB},
  };
  const WatchslingerAppDescriptor customItems[] = {
      {"Custom", &customA},
      {nullptr, nullptr},
  };

  WatchslingerAppRegistry stockOnly(WatchslingerAppList(stockItems, 4));
  ASSERT_EQ(2, stockOnly.count());
  ASSERT_TRUE(strcmp("About Watchslinger", stockOnly.get(0)->label) == 0);
  ASSERT_TRUE(strcmp("Set Time", stockOnly.get(1)->label) == 0);
  ASSERT_TRUE(stockOnly.get(2) == nullptr);

  WatchslingerAppRegistry combined(WatchslingerAppList(stockItems, 4),
                                   WatchslingerAppList(customItems, 2));
  ASSERT_EQ(3, combined.count());
  ASSERT_TRUE(strcmp("Custom", combined.get(2)->label) == 0);
  ASSERT_TRUE(combined.get(3) == nullptr);

  WatchslingerAppDescriptor valid{"Valid", &stockA};
  WatchslingerAppDescriptor missingLabel{nullptr, &stockA};
  WatchslingerAppDescriptor missingApp{"Missing App", nullptr};
  ASSERT_TRUE(valid.isLaunchable());
  ASSERT_TRUE(!missingLabel.isLaunchable());
  ASSERT_TRUE(!missingApp.isLaunchable());

  Watchslinger watch;
  WatchslingerLegacyApp legacyApp(&Watchslinger::openLegacy);
  ASSERT_TRUE(!watch.legacyOpened);
  legacyApp.onOpen(watch);
  ASSERT_TRUE(watch.legacyOpened);
  ASSERT_TRUE(legacyApp.closeAfterOpen());
  ASSERT_TRUE(!stockA.closeAfterOpen());

  WatchslingerLegacyApp missingLegacyApp(nullptr);
  missingLegacyApp.onOpen(watch);

  printf("all app registry tests passed\n");
  return 0;
}
```

- [ ] **Step 2: Run the registry test to verify it fails**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
```

Expected: failure because `WatchslingerAppRegistry::count()` still includes invalid descriptors.

- [ ] **Step 3: Update registry counting and lookup**

In `src/WatchslingerApp.h`, replace the `WatchslingerAppRegistry` class with:

```cpp
class WatchslingerAppRegistry {
public:
  WatchslingerAppRegistry(WatchslingerAppList primary = WatchslingerAppList(),
                          WatchslingerAppList secondary = WatchslingerAppList())
      : primary_(primary), secondary_(secondary) {}

  uint8_t count() const {
    return countLaunchable(primary_) + countLaunchable(secondary_);
  }

  const WatchslingerAppDescriptor *get(uint8_t index) const {
    uint8_t primaryCount = countLaunchable(primary_);
    if (index < primaryCount) {
      return getLaunchable(primary_, index);
    }
    return getLaunchable(secondary_, index - primaryCount);
  }

private:
  static uint8_t countLaunchable(WatchslingerAppList list) {
    uint8_t total = 0;
    for (uint8_t i = 0; i < list.count; i++) {
      if (list.items[i].isLaunchable()) {
        total++;
      }
    }
    return total;
  }

  static const WatchslingerAppDescriptor *getLaunchable(
      WatchslingerAppList list, uint8_t index) {
    uint8_t launchableIndex = 0;
    for (uint8_t i = 0; i < list.count; i++) {
      if (!list.items[i].isLaunchable()) {
        continue;
      }
      if (launchableIndex == index) {
        return &list.items[i];
      }
      launchableIndex++;
    }
    return nullptr;
  }

  WatchslingerAppList primary_;
  WatchslingerAppList secondary_;
};
```

- [ ] **Step 4: Run registry and weather helper tests**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
g++ -std=c++11 -Isrc test/host/test_weather_helpers.cpp -o /tmp/watchslinger_weather_helpers_test && /tmp/watchslinger_weather_helpers_test
```

Expected: both tests pass.

- [ ] **Step 5: Commit**

Run:

```bash
git add src/WatchslingerApp.h test/host/test_app_registry.cpp
git commit -m "Skip invalid menu descriptors" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 6: Add The Foundation Example And Remove Insecure Weather Example Settings

**Files:**
- Create: `examples/Watchslinger/Foundation/Foundation.ino`
- Create: `examples/Watchslinger/Foundation/settings.h`
- Modify: `platformio.ini`
- Modify: `examples/WatchFaces/7_SEG/settings.h`
- Modify: `examples/WatchFaces/Basic/settings.h`
- Modify: `examples/WatchFaces/DOS/settings.h`
- Modify: `examples/WatchFaces/MacPaint/settings.h`
- Modify: `examples/WatchFaces/Mario/settings.h`
- Modify: `examples/WatchFaces/Pokemon/settings.h`
- Modify: `examples/WatchFaces/StarryHorizon/settings.h`
- Modify: `examples/WatchFaces/Tetris/settings.h`

- [ ] **Step 1: Add the foundation example settings**

Create `examples/Watchslinger/Foundation/settings.h`:

```cpp
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
```

- [ ] **Step 2: Add the foundation example sketch**

Create `examples/Watchslinger/Foundation/Foundation.ino`:

```cpp
#include <Watchslinger.h>
#include <OpenMeteoProvider.h>
#include <OpenWeatherMapProvider.h>
#include "settings.h"

class AboutStockApp : public WatchslingerApp {
public:
  void onOpen(Watchslinger &watch) override { watch.showAbout(); }
  bool closeAfterOpen() const override { return true; }
};

class HelloApp : public WatchslingerApp {
public:
  void onOpen(Watchslinger &watch) override {
    watch.display.setFullWindow();
    watch.display.fillScreen(GxEPD_BLACK);
    watch.display.setTextColor(GxEPD_WHITE);
    watch.display.setCursor(20, 100);
    watch.display.print("Hello app");
    watch.display.display(false);
  }

  bool onButton(Watchslinger &watch, WatchslingerButton button) override {
    if (button == WatchslingerButton::Back) {
      watch.showMenu(menuIndex, false);
      return true;
    }
    return false;
  }
};

class FoundationFace : public Watchslinger {
  using Watchslinger::Watchslinger;

public:
  void drawWatchFace() override {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 80);
    display.print("Watchslinger");
    display.setCursor(10, 110);
    display.print(currentTime.Hour);
    display.print(":");
    if (currentTime.Minute < 10) {
      display.print("0");
    }
    display.print(currentTime.Minute);
    display.setCursor(10, 140);
    display.print(getBatteryVoltage());
    display.print("V");
  }
};

#ifdef WATCHSLINGER_USE_OPENWEATHERMAP
using SelectedWeatherProvider = OpenWeatherMapProvider;
#else
using SelectedWeatherProvider = OpenMeteoProvider;
#endif

SelectedWeatherProvider weatherProvider;
AboutStockApp aboutApp;
HelloApp helloApp;

const WatchslingerAppDescriptor foundationMenuItems[] = {
    {"About", &aboutApp},
    {"Hello", &helloApp},
};
WatchslingerAppList foundationMenu(foundationMenuItems, 2);

FoundationFace watchy(settings, foundationMenu, WatchslingerMenuMode::CustomOnly,
                      weatherProvider);

void setup() { watchy.init(); }

void loop() {}
```

- [ ] **Step 3: Point PlatformIO at the foundation example**

In `platformio.ini`, replace:

```ini
src_dir = examples/Watchslinger/Canonical
```

with:

```ini
src_dir = examples/Watchslinger/Foundation
```

- [ ] **Step 4: Replace weather settings in legacy watchface examples**

For each of these files:

```text
examples/WatchFaces/7_SEG/settings.h
examples/WatchFaces/Basic/settings.h
examples/WatchFaces/DOS/settings.h
examples/WatchFaces/MacPaint/settings.h
examples/WatchFaces/Mario/settings.h
examples/WatchFaces/Pokemon/settings.h
examples/WatchFaces/StarryHorizon/settings.h
examples/WatchFaces/Tetris/settings.h
```

replace the weather block with this Open-Meteo-compatible settings file content, keeping each file's existing include guard name if it has a unique one:

```cpp
#ifndef SETTINGS_H
#define SETTINGS_H

#define LAT "40.7128"
#define LON "-74.0060"
#define TEMP_UNIT "metric"
#define TEMP_LANG "en"
#define WEATHER_UPDATE_INTERVAL 30
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600 * -5

watchySettings settings{
    .cityID = "",
    .lat = LAT,
    .lon = LON,
    .weatherAPIKey = "",
    .weatherURL = "",
    .weatherUnit = TEMP_UNIT,
    .weatherLang = TEMP_LANG,
    .weatherUpdateInterval = WEATHER_UPDATE_INTERVAL,
    .ntpServer = NTP_SERVER,
    .gmtOffset = GMT_OFFSET_SEC,
    .vibrateOClock = true,
};

#endif
```

- [ ] **Step 5: Verify no checked-in HTTP weather URLs or shared API key remain**

Run:

```bash
rg 'http://api\.openweathermap|f058fe1cad2afe8e2ddc5d063a64cecb' examples src README.md
```

Expected: no matches.

- [ ] **Step 6: Commit**

Run:

```bash
git add platformio.ini examples/Watchslinger/Foundation examples/WatchFaces/*/settings.h
git commit -m "Add foundation customization example" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 7: Update README Documentation

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Replace README with Watchslinger documentation**

Replace `README.md` with:

````markdown
# Watchslinger

Watchslinger is custom firmware for Watchy v3. It preserves source-level compatibility with community Watchy watchfaces while making the firmware internals easier to own and customize.

## Target Hardware

Watchslinger targets Watchy v3:

- ESP32-S3
- 8 MB flash
- GDEY0154D67 200x200 e-paper display
- PCF8563 RTC
- BMA423 accelerometer

Use PlatformIO. Arduino IDE project setup is not part of this firmware foundation.

## Build

```sh
pio run
```

The default PlatformIO environment uses `esp32-s3-devkitc-1`, `default_8MB.csv`, USB CDC on boot, and `-DARDUINO_WATCHY_V30`.

## Watchy Compatibility

Existing watchfaces can continue to include `Watchy.h`, subclass `Watchy`, override `drawWatchFace()`, and use familiar members such as `display`, `currentTime`, `getBatteryVoltage()`, and `vibMotor()`.

```cpp
#include <Watchy.h>

class MyFace : public Watchy {
  using Watchy::Watchy;

public:
  void drawWatchFace() override;
};
```

New code can include `Watchslinger.h` and subclass `Watchslinger` directly.

```cpp
#include <Watchslinger.h>

class MyFace : public Watchslinger {
  using Watchslinger::Watchslinger;

public:
  void drawWatchFace() override;
};
```

Swapping the active watchface is a sketch-level instantiation change:

```cpp
MyFace watchy(settings);
```

## Custom Menus

Menus are declared as app descriptors instead of hardcoded firmware strings. Use `WatchslingerApp` for custom entries:

```cpp
class HelloApp : public WatchslingerApp {
public:
  void onOpen(Watchslinger &watch) override {
    watch.display.setFullWindow();
    watch.display.fillScreen(GxEPD_BLACK);
    watch.display.setTextColor(GxEPD_WHITE);
    watch.display.setCursor(20, 100);
    watch.display.print("Hello app");
    watch.display.display(false);
  }
};

HelloApp helloApp;
const WatchslingerAppDescriptor items[] = {
    {"Hello", &helloApp},
};
WatchslingerAppList customMenu(items, 1);
```

Menu modes:

- `WatchslingerMenuMode::StockOnly`: stock entries only
- `WatchslingerMenuMode::StockPlusCustom`: stock entries followed by custom entries
- `WatchslingerMenuMode::CustomOnly`: replace stock entries entirely

```cpp
MyFace watchy(settings, customMenu, WatchslingerMenuMode::StockPlusCustom);
```

## Weather Providers

`getWeatherData()` now delegates to a `WeatherProvider`.

Default behavior uses `OpenMeteoProvider`, which calls `https://api.open-meteo.com/`, requires no API key, and uses latitude/longitude from `watchySettings`.

OpenWeatherMap remains available for users who want compatibility with the old source. It uses HTTPS and requires your own API key. Watchslinger does not ship a shared OpenWeatherMap key.

```cpp
#include <OpenMeteoProvider.h>
#include <OpenWeatherMapProvider.h>

using SelectedWeatherProvider = OpenMeteoProvider;
// using SelectedWeatherProvider = OpenWeatherMapProvider;

SelectedWeatherProvider weatherProvider;
MyFace watchy(settings, customMenu, WatchslingerMenuMode::CustomOnly,
              weatherProvider);
```

All checked-in network URLs must use HTTPS. Do not commit real API keys.

## Foundation Example

See `examples/Watchslinger/Foundation/` for a minimal sketch that demonstrates:

- a custom watchface
- a custom menu with one stock screen wrapper and one custom app
- Open-Meteo default weather
- OpenWeatherMap opt-in with a compile-time key guard

```
examples/Watchslinger/Foundation/Foundation.ino
examples/Watchslinger/Foundation/settings.h
```
````

- [ ] **Step 2: Verify README mentions required topics**

Run:

```bash
rg 'PlatformIO|Watchy v3|Watchy.h|Watchslinger.h|WatchslingerMenuMode|OpenMeteoProvider|OpenWeatherMapProvider|HTTPS|API key|pio run' README.md
```

Expected: each topic appears at least once.

- [ ] **Step 3: Commit**

Run:

```bash
git add README.md
git commit -m "Document Watchslinger customization foundation" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

## Task 8: Run Full Validation

**Files:**
- Validate repository state only.

- [ ] **Step 1: Run all host tests**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
g++ -std=c++11 -Isrc test/host/test_weather_helpers.cpp -o /tmp/watchslinger_weather_helpers_test && /tmp/watchslinger_weather_helpers_test
g++ -std=c++11 -Isrc test/host/test_v3_macro.cpp -o /tmp/watchslinger_v3_macro_test && /tmp/watchslinger_v3_macro_test
g++ -std=c++11 -Isrc test/host/test_esp32s3_macro.cpp -o /tmp/watchslinger_esp32s3_macro_test && /tmp/watchslinger_esp32s3_macro_test
```

Expected: all commands exit 0.

- [ ] **Step 2: Verify networking policy**

Run:

```bash
rg 'http://|f058fe1cad2afe8e2ddc5d063a64cecb' examples src README.md
```

Expected: no matches.

- [ ] **Step 3: Run the acceptance build**

Run:

```bash
pio run
```

Expected: build succeeds for `watchy_v3`. If warnings appear, fix warnings caused by this work. If warnings come from third-party dependencies or unchanged upstream code, record the exact warning text for the final implementation note.

- [ ] **Step 4: Inspect final git state**

Run:

```bash
git status --short
git log --oneline --decorate -8
```

Expected: no uncommitted changes after all implementation commits, and the recent log includes the commits from this plan.
