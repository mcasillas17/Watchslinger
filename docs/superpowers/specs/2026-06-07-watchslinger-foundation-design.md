# Watchslinger Foundation Design

Date: 2026-06-07

## Context

Watchslinger is a fork of `sqfmi/Watchy` that will become a custom firmware while preserving source-level compatibility with community watchfaces. The target hardware is Watchy v3: ESP32-S3, 8 MB flash, GDEY0154D67 200x200 e-paper, PCF8563 RTC, BMA423 accelerometer, vibration motor, and battery ADC.

Watchslinger is firmware, not an operating system. FreeRTOS, ESP-IDF, and arduino-esp32 already provide scheduling, memory management, and peripheral I/O. This foundation defines the application framework above the existing hardware integrations.

The implementation must happen in a fresh worktree based on `origin/master`. The current `origin/master` already contains the earlier Watchslinger menu/watchface foundation merge, so this design consolidates that architecture and extends it to include the weather-provider foundation.

## Goals

- Make watchfaces first-class and source-compatible with community Watchy watchfaces.
- Make the menu declarative, extensible, and replaceable without editing core firmware.
- Replace hardcoded OpenWeatherMap weather fetching with a `WeatherProvider` interface.
- Ship `OpenMeteoProvider` as the default provider and `OpenWeatherMapProvider` as an opt-in compatibility provider.
- Require HTTPS for all network calls.
- Avoid checked-in shared or hardcoded API keys.
- Configure the project for Watchy v3 under PlatformIO.
- Provide at least one minimal example and update README documentation.

## Non-Goals

- New production watchface art or theme content.
- BLE, OTA, accelerometer gestures, or a larger app system beyond the menu foundation.
- Persistent settings storage for menu or weather-provider selection.
- Rewriting the display, RTC, accelerometer, vibration motor, battery ADC, WiFi, NTP, BLE, or OTA HAL code.
- Requiring community watchfaces to rewrite their drawing code.

## Compatibility Contract

Existing community watchfaces that subclass `Watchy` and override `drawWatchFace()` must compile against Watchslinger with at most a one-line include change.

Existing code like this remains valid:

```cpp
#include <Watchy.h>

class MyFace : public Watchy {
  using Watchy::Watchy;

public:
  void drawWatchFace() override;
};
```

New Watchslinger-native code can use the canonical class:

```cpp
#include <Watchslinger.h>

class MyFace : public Watchslinger {
  using Watchslinger::Watchslinger;

public:
  void drawWatchFace() override;
};
```

`Watchy.h` remains available as a compatibility shim that includes the Watchslinger API and exposes `Watchy` as an alias for `Watchslinger`.

The compatibility-facing surface remains available to subclasses:

- `display`
- `currentTime`
- `settings`
- `getBatteryVoltage()`
- `getBoardRevision()`
- `vibMotor()`
- `showWatchFace()`
- `drawWatchFace()`
- `getWeatherData()`

Global symbols already used by community watchfaces, including `guiState`, `menuIndex`, `sensor`, `WIFI_CONFIGURED`, `BLE_CONFIGURED`, and `USB_PLUGGED_IN`, remain available unless a later compatibility audit proves they can be removed safely.

## Architecture

The design uses a layered compatibility refactor.

`Watchslinger` is the canonical firmware class implemented in `Watchslinger.h/.cpp`. `Watchy.h` is a source-compatible shim, not the primary implementation. This gives Watchslinger ownership of internals while preserving the public shape expected by existing watchfaces.

The existing hardware behavior stays intact. GxEPD2 display setup, PCF8563 RTC access, BMA423 setup, vibration motor pin handling, battery ADC reading, WiFi setup, NTP sync, and deep sleep wake handling remain based on the upstream code. The foundation changes happen at the framework layer:

- the watchface remains the special home surface
- the menu becomes a registry-backed app launcher
- weather fetching moves behind a provider interface

The watchface and menu are intentionally separate. The watchface has reset, minute-refresh, and sleep semantics. The menu is an app launcher opened by button input. Weather is a service used by watchfaces and other code through the existing `getWeatherData()` call.

## Watchface Foundation

`Watchslinger::showWatchFace()` remains responsible for setting the full display window, powering the e-paper panel, calling `drawWatchFace()`, refreshing the display, and setting `guiState = WATCHFACE_STATE`.

Swapping the active watchface is a single-line sketch change because the sketch instantiates a different subclass:

```cpp
MyWatchFace watchy(settings);
```

or, in Watchslinger-native code:

```cpp
MyWatchFace watch(settings);
```

No runtime watchface manager is added in this foundation. Runtime watchface switching and persistent watchface selection are outside the current scope.

## Menu Foundation

The menu is represented as static data instead of hardcoded labels and `switch` statements in the firmware class.

### App Interface

Menu-launched experiences implement a small app interface:

```cpp
enum class WatchslingerButton : uint8_t {
  Menu,
  Back,
  Up,
  Down
};

class WatchslingerApp {
public:
  virtual ~WatchslingerApp() = default;
  virtual void onOpen(Watchslinger &watch) = 0;
  virtual void onClose(Watchslinger &watch) {}
  virtual bool onButton(Watchslinger &watch, WatchslingerButton button) { return false; }
  virtual void onTick(Watchslinger &watch) {}
};
```

`onOpen()` is required. Other hooks are optional. `onButton()` returns whether the app handled the button. If an app does not handle Back, Watchslinger closes the app and returns to the menu.

### App Descriptor

An app descriptor is static metadata for one launcher entry:

```cpp
struct WatchslingerAppDescriptor {
  const char *label;
  WatchslingerApp *app;
};
```

The first implementation keeps descriptors lean. IDs, icons, visibility predicates, enabled predicates, and persistent settings can be added later when concrete app needs exist.

Null labels and null app pointers are invalid. The launcher must not dereference invalid descriptors. Invalid entries are skipped when building or traversing the visible menu. If no valid entries remain, the menu shows the same "No apps" screen used for an empty registry.

### App Registry

The registry is a non-owning view over one or two static descriptor lists:

```cpp
struct WatchslingerAppList {
  const WatchslingerAppDescriptor *items;
  uint8_t count;
};

class WatchslingerAppRegistry {
public:
  WatchslingerAppRegistry(WatchslingerAppList primary,
                          WatchslingerAppList secondary = {});

  uint8_t count() const;
  const WatchslingerAppDescriptor *get(uint8_t index) const;
};
```

This avoids heap allocation, avoids C++20-only `std::span`, and supports Arduino-style static configuration.

The menu supports three modes:

- `StockOnly`: stock menu entries only
- `StockPlusCustom`: stock entries followed by custom entries
- `CustomOnly`: custom entries replace stock entries entirely

The default constructor remains source-compatible:

```cpp
Watchy watchy(settings);
```

New code can opt into menu customization with constructor parameters:

```cpp
Watchslinger watch(settings, customApps, WatchslingerMenuMode::StockPlusCustom);
```

Subclass hooks can remain available for advanced firmware builds if they do not complicate the common constructor path.

### Stock Apps

Existing stock screens such as About, Vibrate Motor, Accelerometer, Set Time, Setup WiFi, and Sync NTP are adapted as stock apps. A legacy adapter can call existing blocking member functions from `onOpen()`:

```cpp
using WatchslingerLegacyHandler = void (Watchslinger::*)();
```

This preserves familiar behavior while allowing future apps to use event-driven button handling.

### Empty Menu

If the active registry is empty, pressing the menu button shows a simple "No apps" screen. It must not crash, dereference null pointers, or silently do nothing.

`MENU_LENGTH` must not be the source of truth for menu bounds. Menu movement wraps or clamps against `registry.count()`.

## Weather Foundation

Weather is provided through a `WeatherProvider` interface. `Watchslinger::getWeatherData()` keeps the existing public API and returns the existing `weatherData` struct, but delegates source-specific fetching and parsing to the selected provider.

The interface should be small and Arduino-friendly:

```cpp
class WeatherProvider {
public:
  virtual ~WeatherProvider() = default;
  virtual bool getWeather(const watchySettings &settings, weatherData &weather) = 0;
};
```

The boolean return indicates whether external weather data was fetched and parsed successfully. On provider failure, Watchslinger preserves the existing behavior of using the BMA423 internal temperature as fallback weather data, marking `weather.external = false`.

The existing interval/cache behavior remains in Watchslinger:

- `weatherIntervalCounter` starts at `-1`
- requests happen only after `weatherUpdateInterval` minutes
- successful external data updates `currentWeather`
- failed external requests fall back visibly to local/internal data rather than fabricating remote data

### OpenMeteoProvider

`OpenMeteoProvider` is the default provider in checked-in examples and the baseline build.

It uses only HTTPS requests to `https://api.open-meteo.com/`. It requires no API key. It uses latitude and longitude from `watchySettings`; if they are missing, the provider reports failure instead of making a malformed request.

The provider requests current temperature, current weather code, and daily sunrise/sunset data in the same Open-Meteo request so the existing `weatherData` fields remain useful. Open-Meteo weather codes are mapped into Watchy-compatible condition codes and short descriptions as closely as practical. Exact parity with OpenWeatherMap codes is not required, but common conditions such as clear, cloudy, rain, snow, fog, and thunderstorm should map predictably.

### OpenWeatherMapProvider

`OpenWeatherMapProvider` preserves the existing OpenWeatherMap v2.5 behavior for users who opt in.

It uses only HTTPS URLs under `https://api.openweathermap.org/data/2.5/weather`. It supports either city ID or latitude/longitude selection, matching existing settings behavior. It parses the existing response fields:

- `main.temp`
- `weather[0].id`
- `weather[0].main`
- `sys.sunrise`
- `sys.sunset`
- `timezone`

The provider never ships a shared API key. If the user selects OpenWeatherMap without a real key, configuration should fail loudly. The preferred example pattern is a compile-time error for a placeholder key in the selected provider path. Runtime construction should also treat an empty key or known placeholder as provider failure.

### Provider Selection

Provider selection is compile-time friendly. The default config selects OpenMeteo. Users can change one line in an example or config header to select OpenWeatherMap and supply their own key.

Persistent provider selection is out of scope. Runtime provider switching is out of scope.

## Network Policy

All checked-in network URLs must use HTTPS.

No checked-in file may contain a shared or real OpenWeatherMap API key. Examples may include placeholders only when those placeholders fail loudly if selected. The default example path should avoid API keys by using OpenMeteo.

Provider implementations should reject non-HTTPS endpoint strings rather than silently making cleartext requests.

## PlatformIO And Watchy v3

`platformio.ini` targets Watchy v3:

- `board = esp32-s3-devkitc-1`
- `board_build.mcu = esp32s3`
- 8 MB flash
- `default_8MB.csv` partitions
- `-DARDUINO_USB_MODE=1`
- `-DARDUINO_USB_CDC_ON_BOOT=1`
- `-DARDUINO_WATCHY_V30`

The code normalizes v3 checks so the PlatformIO macro and upstream-style ESP32-S3 board macro both select the Watchy v3 path. A small internal macro such as `WATCHSLINGER_V3` is preferred:

```cpp
#if defined(ARDUINO_WATCHY_V30) || defined(ARDUINO_ESP32S3_DEV)
#define WATCHSLINGER_V3
#endif
```

This avoids rewriting v3 HAL logic while making the PlatformIO target reliable.

## Example

Add at least one minimal example under `examples/` that demonstrates:

- a custom watchface declaration
- instantiating that watchface as the active firmware object
- a custom menu using `StockPlusCustom`
- one stock menu item mixed with one custom item
- OpenMeteo as the default provider
- commented OpenWeatherMap opt-in with a loud placeholder key guard

The example should be small enough to act as documentation. It should not depend on Spider-Man art, BLE, OTA, persistent settings, or hardware-specific gestures beyond the standard Watchy buttons.

## README Updates

Replace the upstream-only README with Watchslinger-oriented documentation covering:

- what Watchslinger is
- the Watchy compatibility contract
- PlatformIO build command
- Watchy v3 target configuration
- how to create and select a custom watchface
- how to declare menu apps
- how to choose stock-only, stock-plus-custom, or custom-only menus
- how to select OpenMeteo or OpenWeatherMap
- why OpenWeatherMap requires the user's own key
- the HTTPS/no-shared-key policy

## Testing And Validation

The acceptance command is:

```sh
pio run
```

The implementation should also preserve or add lightweight host/compile checks where practical for:

- `Watchy.h` compatibility alias
- custom watchface compilation
- stock-only registry count
- stock-plus-custom registry count
- custom-only registry count
- empty registry behavior
- OpenWeatherMap placeholder rejection
- HTTPS endpoint enforcement

Warnings found during `pio run` should be fixed when caused by this work. Warnings from existing dependencies or intentionally deferred upstream code should be reported with the final implementation notes.

## Implementation Order

1. Create the implementation branch/worktree from `origin/master`.
2. Confirm existing Watchslinger menu/watchface foundation state on that base.
3. Add or adjust PlatformIO Watchy v3 configuration.
4. Preserve the `Watchy.h` compatibility shim and canonical `Watchslinger` class.
5. Complete or adjust the registry-backed menu API.
6. Add `WeatherProvider`, `OpenMeteoProvider`, and `OpenWeatherMapProvider`.
7. Route `getWeatherData()` through the selected provider while preserving fallback behavior.
8. Remove checked-in shared API keys and HTTP weather URLs from examples.
9. Add the minimal foundation example.
10. Update README documentation.
11. Run `pio run` and address failures.
