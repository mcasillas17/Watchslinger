# Watchslinger Menu And Watchface Foundation Design

Date: 2026-06-05

## Context

Watchslinger is a fork of `sqfmi/Watchy` that will become a custom firmware while preserving source-level compatibility with community watchfaces. The target hardware is Watchy v3 with ESP32-S3, 8 MB flash, GDEY0154D67 200x200 e-paper, PCF8563 RTC, and BMA423 accelerometer.

This design stays at the application framework layer. It does not rewrite the display, RTC, accelerometer, battery, WiFi, NTP, BLE, or OTA hardware integrations.

The current checkout is still close to upstream. The main compatibility surface is `src/Watchy.h` and `src/Watchy.cpp`; community watchfaces subclass `Watchy`, inherit its constructor with `using Watchy::Watchy`, override `drawWatchFace()`, and directly use members such as `display`, `currentTime`, `getBatteryVoltage()`, and `vibMotor()`. The existing menu labels and selection behavior are hardcoded in `Watchy.cpp` and duplicated between normal and fast menu paths.

The current checkout does not include `platformio.ini`. The target build configuration is expected to define `ARDUINO_WATCHY_V30`; existing upstream-style code currently checks `ARDUINO_ESP32S3_DEV` for Watchy v3 paths.

## Goals

- Make `Watchslinger` the canonical firmware class.
- Keep `Watchy.h` source-compatible for existing community watchfaces.
- Keep watchfaces as first-class customizable home surfaces.
- Replace the hardcoded stock menu with an extensible, replaceable app launcher.
- Keep stock menu behavior available by default.
- Use static compile-time tables by default, avoiding dynamic allocation and lifetime traps.
- Define a full app lifecycle shape now, while allowing legacy blocking stock screens to be adapted during migration.

## Non-Goals

- Implement new end-user apps.
- Rework BLE, OTA, accelerometer gestures, networking, weather, RTC, display, or battery HAL internals.
- Build a scheduler, process model, or operating-system-like runtime.
- Require community watchfaces to rewrite their drawing code.

## Compatibility Contract

`Watchslinger` becomes the real firmware class. `Watchy.h` remains as a compatibility shim that includes the new API and exposes `Watchy` as an alias for `Watchslinger`.

Existing code such as this should continue to compile:

```cpp
#include <Watchy.h>

class MyFace : public Watchy {
  using Watchy::Watchy;

public:
  void drawWatchFace() override;
};
```

New Watchslinger-native code can use the canonical name:

```cpp
#include <Watchslinger.h>

class MyFace : public Watchslinger {
  using Watchslinger::Watchslinger;

public:
  void drawWatchFace() override;
};
```

The compatibility-facing members remain available to subclasses:

- `display`
- `currentTime`
- `settings`
- `getBatteryVoltage()`
- `getBoardRevision()`
- `vibMotor()`
- `showWatchFace()`
- `drawWatchFace()`

If an implementation choice threatens this contract, it must be called out before proceeding.

## Architecture

The watchface stays separate from the app system. It is the home surface with special wake, sleep, and minute-refresh semantics. `Watchslinger::showWatchFace()` remains responsible for setting the display window, powering the e-paper panel, calling `drawWatchFace()`, refreshing the display, and setting `guiState = WATCHFACE_STATE`.

The menu becomes an app launcher. It renders an active static app registry, tracks the selected entry, and dispatches button events to the selected app. Stock apps are present by default. Custom firmware can keep stock apps, append custom apps, or replace the registry entirely.

The app model is event-driven. Apps receive lifecycle and input events through a small interface. Existing blocking Watchy-style screens can be wrapped by a legacy adapter so the architecture can move forward without rewriting every stock screen in the first implementation.

## Components

### Watchslinger

`Watchslinger` owns firmware-level behavior:

- initialization and wake-reason handling
- deep sleep setup
- watchface rendering
- shared hardware helpers
- settings and compatibility-facing state
- routing button events between watchface, menu, and current app

It delegates menu rendering, index bounds, and app dispatch to launcher components. Hardcoded menu label arrays and duplicated `switch (menuIndex)` dispatch should move out of the firmware class.

### Watchy.h Compatibility Shim

`Watchy.h` should remain available for source compatibility. It should include the Watchslinger API and expose:

```cpp
using Watchy = Watchslinger;
```

Any global symbols already used by community watchfaces, such as `guiState`, `menuIndex`, `sensor`, `WIFI_CONFIGURED`, `BLE_CONFIGURED`, and `USB_PLUGGED_IN`, should remain available unless a later compatibility audit proves they are not part of real-world watchface usage.

### WatchslingerApp

`WatchslingerApp` is the app lifecycle interface for menu-launched experiences.

Proposed shape:

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

`onOpen()` is required. The other hooks are optional. `onButton()` returns whether the app handled the button.

The exact destructor strategy should be validated against Arduino/ESP32 compiler settings. If virtual destructors create avoidable overhead or warnings, the implementation can use a callback table with the same lifecycle semantics instead.

### WatchslingerAppDescriptor

An app descriptor is static metadata for one launcher entry.

Proposed initial shape:

```cpp
struct WatchslingerAppDescriptor {
  const char *label;
  WatchslingerApp *app;
};
```

The first implementation should keep this lean. IDs, icons, visibility predicates, enabled predicates, and persistent settings can be added later when there is a concrete app need.

Null `app` pointers are invalid for launch. The launcher must not dereference them.

### WatchslingerAppRegistry

The registry is a non-owning view over static app descriptors.

It should support:

- stock-only registry
- custom-only registry
- stock-plus-custom registry
- empty registry

The registry should not allocate memory. It can be represented as one or two descriptor spans using pointer-plus-count pairs, since C++20 `std::span` is not available.

Proposed shape:

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

For stock-plus-custom, one list can hold stock descriptors and the other custom descriptors. For custom-only, only the custom list is supplied. For stock-only, only the stock list is supplied.

### WatchslingerMenu

The menu renders the active registry as a selectable list and owns selection behavior.

Responsibilities:

- render labels using the existing GxEPD2 display stack
- track the selected index
- wrap or clamp up/down against `registry.count()`
- launch the selected descriptor
- handle empty registries
- eliminate dependence on `MENU_LENGTH`

Rendering can initially match the stock Watchy menu style to keep behavior familiar. Layout customization is out of scope for this foundation.

### Legacy Blocking App Adapter

Existing stock screens such as About, Buzz, Accelerometer, Set Time, Setup WiFi, and Sync NTP can be adapted without rewriting them immediately.

Proposed shape:

```cpp
using WatchslingerLegacyHandler = void (Watchslinger::*)();

class WatchslingerLegacyApp : public WatchslingerApp {
public:
  WatchslingerLegacyApp(WatchslingerLegacyHandler handler);
  void onOpen(Watchslinger &watch) override;
};
```

The adapter calls the blocking member handler from `onOpen()`, such as `watch.showAbout()` or `watch.setTime()`. After the handler returns, the launcher returns to the menu or watchface according to the behavior of that handler. This preserves current stock behavior while allowing future apps to become event-driven.

## Public Customization API

The design uses a hybrid A+B model:

- Declarative static app tables are the common path.
- Subclass hooks are the escape hatch for dynamic or advanced behavior.

Constructor-oriented customization can support static registries:

```cpp
Watchslinger watch(settings, customApps);
Watchslinger watch(settings, customApps, WatchslingerMenuMode::StockPlusCustom);
```

Subclass-oriented customization can expose virtual hooks:

```cpp
class MyFirmware : public Watchslinger {
  using Watchslinger::Watchslinger;

protected:
  WatchslingerAppRegistry appRegistry() override;
};
```

The implementation plan should choose the smallest API that supports these modes without introducing ambiguous overloads.

Menu modes:

- `StockOnly`: default behavior
- `StockPlusCustom`: append custom apps after stock apps
- `CustomOnly`: replace stock apps entirely

The default constructor remains compatible with existing sketches:

```cpp
Watchy watchy(settings);
```

## Data Flow

On reset or timer wake, `Watchslinger::init()` initializes I2C, RTC, display, and wake handling as the upstream code does today. It reads `currentTime` and calls `showWatchFace()` for watchface display.

On a minute tick while in watchface state, the watch reads time, refreshes the watchface partially, and keeps the existing hourly vibration behavior.

On menu button from the watchface, `Watchslinger` opens `WatchslingerMenu`. The menu reads from the active app registry and renders the available app labels.

When up or down is pressed in menu state, the menu updates the selected index against the registry count and redraws. It no longer uses `MENU_LENGTH`.

When menu/select is pressed in menu state, the launcher opens the selected app and calls `onOpen(watch)`.

When an event-driven app is active, subsequent button presses are routed to `onButton(watch, button)`. If the app does not handle back, Watchslinger closes the app and returns to the menu.

When a legacy blocking app is active, `onOpen()` calls the wrapped stock handler. The existing handler can block until it finishes, then return to the menu or watchface using current behavior. This is a migration bridge, not the preferred shape for new apps.

The existing fast-menu loop can remain initially, but it should route through the menu and launcher rather than duplicating app selection dispatch.

## Empty And Invalid States

If the active registry is empty, pressing the menu button should show a simple "No apps" screen instead of crashing or silently doing nothing. This makes custom-only registry mistakes visible on-device.

Invalid selected indices should be wrapped or clamped against the active registry count. They must never be compared against `MENU_LENGTH`.

Descriptors with null labels should render as a blank or placeholder label only if explicitly allowed by implementation. The safer first implementation is to treat null labels as invalid.

Descriptors with null app pointers are non-launchable. The launcher must not dereference them.

## Watchy v3 Build Macro Normalization

The project target is Watchy v3 under PlatformIO with `-DARDUINO_WATCHY_V30`. Current upstream-style code checks `ARDUINO_ESP32S3_DEV` for v3 behavior.

The implementation should normalize this before relying on v3 code paths. Acceptable approaches:

- update v3 guards to check `ARDUINO_WATCHY_V30`
- define an internal `WATCHSLINGER_V3` macro when either `ARDUINO_WATCHY_V30` or `ARDUINO_ESP32S3_DEV` is defined

The second option is preferred because it preserves compatibility with existing Arduino board definitions while supporting the PlatformIO target macro.

## Testing Strategy

### Compile Compatibility

- Existing example watchfaces compile with `#include <Watchy.h>` and no source changes.
- At least one example compiles with `#include <Watchslinger.h>` and `class Face : public Watchslinger`.
- Compatibility examples preserve `using Watchy::Watchy` inheritance.

### Registry Behavior

Where practical, add lightweight C++ tests or compile-time examples for:

- empty registry count
- stock registry count
- custom-only registry
- stock-plus-custom registry
- index wrap or clamp
- null handler behavior

### Firmware Behavior

Build a Watchy v3 PlatformIO target with `-DARDUINO_WATCHY_V30`.

Manual or hardware-backed verification should cover:

- boot to watchface
- minute watchface refresh
- menu open from watchface
- up/down wrapping
- stock app launch
- back behavior
- one custom app descriptor
- custom-only registry with no apps shows "No apps"

## Migration Notes

The first implementation should avoid broad unrelated refactors. A practical migration order is:

1. Add `Watchslinger` headers and compatibility shim.
2. Move or rename the core class while preserving public members.
3. Add app lifecycle types and registry views.
4. Add stock app descriptors using legacy adapters.
5. Replace hardcoded menu rendering and dispatch with registry-backed launcher behavior.
6. Normalize Watchy v3 build macros.
7. Add compile/build examples for compatibility and new API usage.

Generated binaries and image assets currently dirty in the working tree are unrelated to this design and should not be modified by this work.
