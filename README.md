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
