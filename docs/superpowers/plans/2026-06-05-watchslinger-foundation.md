# Watchslinger Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the Watchslinger watchface compatibility layer and static app/menu foundation described in the approved design.

**Architecture:** `Watchslinger` becomes the canonical firmware class while `Watchy.h` remains a compatibility alias. The watchface stays as the home surface, and the menu becomes a launcher over static app descriptors with lifecycle hooks and a legacy blocking adapter for current stock screens.

**Tech Stack:** Arduino-style C++11, arduino-esp32, PlatformIO, GxEPD2, existing Watchy RTC/BMA/WiFi integrations, small host `g++` tests for registry-only code.

---

## Execution Context

- Worktree: `/Users/elopenmike/build/DIY/Watchy/Watchslinger/.worktrees/watchslinger-foundation`
- Branch: `watchslinger-foundation`
- Do not add contributor, AI, assistant, or tool attribution to commits, docs, examples, or generated text.
- Keep the generated watchface binaries and images out of every commit.

## File Structure

- Create `platformio.ini`: Watchy v3 PlatformIO environment and library dependencies.
- Create `src/WatchslingerApp.h`: host-testable app lifecycle, descriptor, list, registry, menu mode, and legacy adapter declarations.
- Create `src/WatchslingerMenu.h`: menu launcher interface.
- Create `src/WatchslingerMenu.cpp`: e-paper menu rendering, empty-registry screen, selection wrapping, and app launch dispatch.
- Create `src/Watchslinger.h`: canonical firmware class, derived from the current `Watchy.h` contents.
- Move `src/Watchy.cpp` to `src/Watchslinger.cpp`: canonical implementation file.
- Modify `src/Watchy.h`: compatibility shim that includes `Watchslinger.h` and aliases `Watchy`.
- Modify `src/config.h`: normalize `ARDUINO_WATCHY_V30` and `ARDUINO_ESP32S3_DEV` into `WATCHSLINGER_V3`.
- Modify `src/Display.cpp`: use `WATCHSLINGER_V3`.
- Modify `examples/WatchFaces/7_SEG/Watchy_7_SEG.cpp`: use `WATCHSLINGER_V3`.
- Create `test/host/test_app_registry.cpp`: host test for static registry behavior.
- Create `test/host/test_v3_macro.cpp`: host test for Watchy v3 macro normalization.
- Create `examples/Watchslinger/Canonical/Canonical.ino`: compile smoke for canonical `Watchslinger` include/class.
- Create `examples/Watchslinger/Canonical/settings.h`: settings for the canonical smoke example.
- Create `examples/Watchslinger/CustomApp/CustomApp.ino`: compile smoke for custom app descriptors.
- Create `examples/Watchslinger/CustomApp/settings.h`: settings for the custom app smoke example.

## Task 1: Add Host-Tested App Registry Primitives

**Files:**
- Create: `src/WatchslingerApp.h`
- Create: `test/host/test_app_registry.cpp`

- [ ] **Step 1: Write the failing host registry test**

Create `test/host/test_app_registry.cpp`:

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

class DummyApp : public WatchslingerApp {
public:
  void onOpen(Watchslinger &) override {}
};

int main() {
  WatchslingerAppList emptyList;
  WatchslingerAppRegistry emptyRegistry(emptyList);
  ASSERT_EQ(0, emptyRegistry.count());
  ASSERT_TRUE(emptyRegistry.get(0) == nullptr);

  DummyApp stockA;
  DummyApp stockB;
  DummyApp customA;

  const WatchslingerAppDescriptor stockItems[] = {
      {"About Watchslinger", &stockA},
      {"Set Time", &stockB},
  };
  const WatchslingerAppDescriptor customItems[] = {
      {"Custom", &customA},
  };

  WatchslingerAppRegistry stockOnly(
      WatchslingerAppList(stockItems, 2));
  ASSERT_EQ(2, stockOnly.count());
  ASSERT_TRUE(strcmp("About Watchslinger", stockOnly.get(0)->label) == 0);
  ASSERT_TRUE(strcmp("Set Time", stockOnly.get(1)->label) == 0);
  ASSERT_TRUE(stockOnly.get(2) == nullptr);

  WatchslingerAppRegistry combined(
      WatchslingerAppList(stockItems, 2),
      WatchslingerAppList(customItems, 1));
  ASSERT_EQ(3, combined.count());
  ASSERT_TRUE(strcmp("Custom", combined.get(2)->label) == 0);
  ASSERT_TRUE(combined.get(3) == nullptr);

  WatchslingerAppDescriptor valid{"Valid", &stockA};
  WatchslingerAppDescriptor missingLabel{nullptr, &stockA};
  WatchslingerAppDescriptor missingApp{"Missing App", nullptr};
  ASSERT_TRUE(valid.isLaunchable());
  ASSERT_TRUE(!missingLabel.isLaunchable());
  ASSERT_TRUE(!missingApp.isLaunchable());

  printf("all app registry tests passed\n");
  return 0;
}
```

- [ ] **Step 2: Run the host registry test to verify it fails**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test
```

Expected: compile fails with `fatal error: '../../src/WatchslingerApp.h' file not found` or equivalent.

- [ ] **Step 3: Add the app lifecycle and registry header**

Create `src/WatchslingerApp.h`:

```cpp
#ifndef WATCHSLINGER_APP_H
#define WATCHSLINGER_APP_H

#include <stdint.h>

class Watchslinger;

enum class WatchslingerButton : uint8_t {
  Menu,
  Back,
  Up,
  Down
};

class WatchslingerApp {
public:
  virtual ~WatchslingerApp() {}
  virtual void onOpen(Watchslinger &watch) = 0;
  virtual void onClose(Watchslinger &watch) {}
  virtual bool onButton(Watchslinger &watch, WatchslingerButton button) {
    (void)watch;
    (void)button;
    return false;
  }
  virtual void onTick(Watchslinger &watch) { (void)watch; }
};

struct WatchslingerAppDescriptor {
  const char *label;
  WatchslingerApp *app;

  bool isLaunchable() const { return label != nullptr && app != nullptr; }
};

struct WatchslingerAppList {
  const WatchslingerAppDescriptor *items;
  uint8_t count;

  WatchslingerAppList() : items(nullptr), count(0) {}
  WatchslingerAppList(const WatchslingerAppDescriptor *appItems,
                      uint8_t appCount)
      : items(appItems), count(appCount) {}
};

enum class WatchslingerMenuMode : uint8_t {
  StockOnly,
  StockPlusCustom,
  CustomOnly
};

class WatchslingerAppRegistry {
public:
  WatchslingerAppRegistry(WatchslingerAppList primary = WatchslingerAppList(),
                          WatchslingerAppList secondary = WatchslingerAppList())
      : primary_(primary), secondary_(secondary) {}

  uint8_t count() const { return primary_.count + secondary_.count; }

  const WatchslingerAppDescriptor *get(uint8_t index) const {
    if (index < primary_.count) {
      return &primary_.items[index];
    }
    index -= primary_.count;
    if (index < secondary_.count) {
      return &secondary_.items[index];
    }
    return nullptr;
  }

private:
  WatchslingerAppList primary_;
  WatchslingerAppList secondary_;
};

using WatchslingerLegacyHandler = void (Watchslinger::*)();

class WatchslingerLegacyApp : public WatchslingerApp {
public:
  explicit WatchslingerLegacyApp(WatchslingerLegacyHandler handler)
      : handler_(handler) {}

  void onOpen(Watchslinger &watch) override;

private:
  WatchslingerLegacyHandler handler_;
};

#endif
```

- [ ] **Step 4: Run the host registry test to verify it passes**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
```

Expected:

```text
all app registry tests passed
```

- [ ] **Step 5: Commit**

```bash
git add src/WatchslingerApp.h test/host/test_app_registry.cpp
git commit -m "Add Watchslinger app registry primitives"
```

## Task 2: Add Watchy v3 Macro Normalization And PlatformIO Config

**Files:**
- Create: `platformio.ini`
- Create: `test/host/test_v3_macro.cpp`
- Modify: `src/config.h`
- Modify: `src/Watchy.h`
- Modify: `src/Watchy.cpp`
- Modify: `src/Display.cpp`
- Modify: `examples/WatchFaces/7_SEG/Watchy_7_SEG.cpp`

- [ ] **Step 1: Write the failing v3 macro host test**

Create `test/host/test_v3_macro.cpp`:

```cpp
#define ARDUINO_WATCHY_V30

#include "../../src/config.h"

#ifndef WATCHSLINGER_V3
#error "ARDUINO_WATCHY_V30 must enable WATCHSLINGER_V3"
#endif

#ifndef WATCHY_V3_SDA
#error "WATCHSLINGER_V3 must select Watchy v3 pins"
#endif

#if MENU_BTN_PIN != 7
#error "Watchy v3 menu button pin should be 7"
#endif

#if BACK_BTN_PIN != 6
#error "Watchy v3 back button pin should be 6"
#endif

int main() { return 0; }
```

- [ ] **Step 2: Run the macro test to verify it fails**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_v3_macro.cpp -o /tmp/watchslinger_v3_macro_test
```

Expected: compile fails with `ARDUINO_WATCHY_V30 must enable WATCHSLINGER_V3`.

- [ ] **Step 3: Add the normalized v3 macro**

In `src/config.h`, immediately after `#define CONFIG_H`, add:

```cpp
#if defined(ARDUINO_WATCHY_V30) || defined(ARDUINO_ESP32S3_DEV)
  #ifndef WATCHSLINGER_V3
    #define WATCHSLINGER_V3
  #endif
#endif
```

In `src/config.h`, change:

```cpp
#ifdef ARDUINO_ESP32S3_DEV //V3
```

to:

```cpp
#ifdef WATCHSLINGER_V3 //V3
```

- [ ] **Step 4: Replace remaining v3 guards**

Run:

```bash
perl -0pi -e 's/ARDUINO_ESP32S3_DEV/WATCHSLINGER_V3/g' src/Watchy.h src/Watchy.cpp src/Display.cpp examples/WatchFaces/7_SEG/Watchy_7_SEG.cpp
```

Expected: `rg "ARDUINO_ESP32S3_DEV" src examples/WatchFaces/7_SEG` prints no matches.

- [ ] **Step 5: Add PlatformIO configuration**

Create `platformio.ini`:

```ini
[platformio]
default_envs = watchy_v3

[env:watchy_v3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.flash_size = 8MB
board_upload.flash_size = 8MB
monitor_speed = 115200
lib_ldf_mode = deep+
build_flags =
  -DARDUINO_WATCHY_V30
  -DCORE_DEBUG_LEVEL=0
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

- [ ] **Step 6: Run host macro test and PlatformIO compile smoke**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_v3_macro.cpp -o /tmp/watchslinger_v3_macro_test && /tmp/watchslinger_v3_macro_test
pio ci examples/WatchFaces/Basic/Basic.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: host macro test exits with status 0. PlatformIO compiles `Basic.ino` for `watchy_v3`.

- [ ] **Step 7: Commit**

```bash
git add platformio.ini test/host/test_v3_macro.cpp src/config.h src/Watchy.h src/Watchy.cpp src/Display.cpp examples/WatchFaces/7_SEG/Watchy_7_SEG.cpp
git commit -m "Normalize Watchy v3 PlatformIO builds"
```

## Task 3: Introduce Canonical Watchslinger Class And Watchy Shim

**Files:**
- Create: `src/Watchslinger.h`
- Move: `src/Watchy.cpp` to `src/Watchslinger.cpp`
- Modify: `src/Watchy.h`
- Create: `examples/Watchslinger/Canonical/Canonical.ino`
- Create: `examples/Watchslinger/Canonical/settings.h`

- [ ] **Step 1: Add the failing canonical include smoke example**

Create `examples/Watchslinger/Canonical/settings.h`:

```cpp
#ifndef WATCHSLINGER_CANONICAL_SETTINGS_H
#define WATCHSLINGER_CANONICAL_SETTINGS_H

watchySettings settings{
    .cityID = "",
    .lat = "",
    .lon = "",
    .weatherAPIKey = "",
    .weatherURL = "",
    .weatherUnit = "metric",
    .weatherLang = "en",
    .weatherUpdateInterval = 30,
    .ntpServer = "pool.ntp.org",
    .gmtOffset = 0,
    .vibrateOClock = true,
};

#endif
```

Create `examples/Watchslinger/Canonical/Canonical.ino`:

```cpp
#include <Watchslinger.h>
#include "settings.h"

class CanonicalFace : public Watchslinger {
  using Watchslinger::Watchslinger;

public:
  void drawWatchFace() override {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 100);
    display.print("Watchslinger");
  }
};

CanonicalFace watchy(settings);

void setup() {
  watchy.init();
}

void loop() {}
```

- [ ] **Step 2: Run the canonical compile smoke to verify it fails**

Run:

```bash
pio ci examples/Watchslinger/Canonical/Canonical.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: compile fails because `Watchslinger.h` does not exist.

- [ ] **Step 3: Create the canonical header and implementation**

Run:

```bash
cp src/Watchy.h src/Watchslinger.h
git mv src/Watchy.cpp src/Watchslinger.cpp
perl -0pi -e 's/WATCHY_H/WATCHSLINGER_H/g; s/class Watchy\b/class Watchslinger/g; s/explicit Watchy/explicit Watchslinger/g' src/Watchslinger.h
perl -0pi -e 's/#include "Watchy.h"/#include "Watchslinger.h"/; s/\bWatchy::/Watchslinger::/g; s/GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> Watchy::display/GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> Watchslinger::display/g' src/Watchslinger.cpp
```

Replace all contents of `src/Watchy.h` with:

```cpp
#ifndef WATCHY_H
#define WATCHY_H

#include "Watchslinger.h"

using Watchy = Watchslinger;

#endif
```

- [ ] **Step 4: Run canonical and compatibility compile smokes**

Run:

```bash
pio ci examples/Watchslinger/Canonical/Canonical.ino --lib . --project-conf platformio.ini -e watchy_v3
pio ci examples/WatchFaces/7_SEG/7_SEG.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: both examples compile. `7_SEG.ino` still uses `<Watchy.h>`, `class Watchy7SEG : public Watchy`, and `using Watchy::Watchy`.

- [ ] **Step 5: Commit**

```bash
git add src/Watchslinger.h src/Watchslinger.cpp src/Watchy.h examples/Watchslinger/Canonical/Canonical.ino examples/Watchslinger/Canonical/settings.h
git commit -m "Introduce Watchslinger compatibility class"
```

## Task 4: Add Stock App Registry And Custom App Constructor API

**Files:**
- Modify: `src/Watchslinger.h`
- Modify: `src/Watchslinger.cpp`
- Create: `examples/Watchslinger/CustomApp/CustomApp.ino`
- Create: `examples/Watchslinger/CustomApp/settings.h`

- [ ] **Step 1: Add the failing custom app compile smoke**

Create `examples/Watchslinger/CustomApp/settings.h`:

```cpp
#ifndef WATCHSLINGER_CUSTOM_APP_SETTINGS_H
#define WATCHSLINGER_CUSTOM_APP_SETTINGS_H

watchySettings settings{
    .cityID = "",
    .lat = "",
    .lon = "",
    .weatherAPIKey = "",
    .weatherURL = "",
    .weatherUnit = "metric",
    .weatherLang = "en",
    .weatherUpdateInterval = 30,
    .ntpServer = "pool.ntp.org",
    .gmtOffset = 0,
    .vibrateOClock = true,
};

#endif
```

Create `examples/Watchslinger/CustomApp/CustomApp.ino`:

```cpp
#include <Watchslinger.h>
#include "settings.h"

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

HelloApp helloApp;
const WatchslingerAppDescriptor customAppDescriptors[] = {
    {"Hello", &helloApp},
};
WatchslingerAppList customApps(customAppDescriptors, 1);

Watchslinger watchy(settings, customApps,
                    WatchslingerMenuMode::StockPlusCustom);

void setup() {
  watchy.init();
}

void loop() {}
```

- [ ] **Step 2: Run the custom app compile smoke to verify it fails**

Run:

```bash
pio ci examples/Watchslinger/CustomApp/CustomApp.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: compile fails because `Watchslinger` does not have the custom app constructor or registry API yet.

- [ ] **Step 3: Add app members and constructors to `Watchslinger.h`**

Add this include with the other local includes in `src/Watchslinger.h`:

```cpp
#include "WatchslingerApp.h"
```

Replace the current constructor with:

```cpp
  explicit Watchslinger(const watchySettings &s)
      : settings(s), customApps_(), menuMode_(WatchslingerMenuMode::StockOnly),
        activeApp_(nullptr) {}
  Watchslinger(const watchySettings &s, WatchslingerAppList customApps,
               WatchslingerMenuMode menuMode = WatchslingerMenuMode::StockPlusCustom)
      : settings(s), customApps_(customApps), menuMode_(menuMode),
        activeApp_(nullptr) {}
```

Add these public methods near the menu methods:

```cpp
  void openApp(const WatchslingerAppDescriptor *descriptor);
  void closeActiveApp();
  bool dispatchActiveAppButton(WatchslingerButton button);
```

Add this protected virtual hook before `private:`:

```cpp
protected:
  virtual WatchslingerAppRegistry appRegistry();
```

Add these private members after the existing private method declarations:

```cpp
  WatchslingerAppList customApps_;
  WatchslingerMenuMode menuMode_;
  WatchslingerApp *activeApp_;
```

- [ ] **Step 4: Add stock apps and registry selection to `Watchslinger.cpp`**

After the `RTC_DATA_ATTR` globals in `src/Watchslinger.cpp`, add:

```cpp
namespace {

uint8_t appCount(const WatchslingerAppDescriptor *items, uint8_t count) {
  (void)items;
  return count;
}

WatchslingerLegacyApp aboutApp(&Watchslinger::showAbout);
WatchslingerLegacyApp buzzApp(&Watchslinger::showBuzz);
WatchslingerLegacyApp accelerometerApp(&Watchslinger::showAccelerometer);
WatchslingerLegacyApp setTimeApp(&Watchslinger::setTime);
WatchslingerLegacyApp setupWifiApp(&Watchslinger::setupWifi);
WatchslingerLegacyApp syncNtpApp(&Watchslinger::showSyncNTP);

const WatchslingerAppDescriptor stockAppDescriptors[] = {
    {"About Watchslinger", &aboutApp},
    {"Vibrate Motor", &buzzApp},
    {"Show Accelerometer", &accelerometerApp},
    {"Set Time", &setTimeApp},
    {"Setup WiFi", &setupWifiApp},
    {"Sync NTP", &syncNtpApp},
};

const WatchslingerAppList stockApps(
    stockAppDescriptors,
    appCount(stockAppDescriptors,
             sizeof(stockAppDescriptors) / sizeof(stockAppDescriptors[0])));

} // namespace
```

Add these method definitions before `Watchslinger::init`:

```cpp
void WatchslingerLegacyApp::onOpen(Watchslinger &watch) {
  if (handler_ != nullptr) {
    (watch.*handler_)();
  }
  watch.closeActiveApp();
}

WatchslingerAppRegistry Watchslinger::appRegistry() {
  switch (menuMode_) {
  case WatchslingerMenuMode::CustomOnly:
    return WatchslingerAppRegistry(customApps_);
  case WatchslingerMenuMode::StockPlusCustom:
    return WatchslingerAppRegistry(stockApps, customApps_);
  case WatchslingerMenuMode::StockOnly:
  default:
    return WatchslingerAppRegistry(stockApps);
  }
}
```

- [ ] **Step 5: Add app open and close methods**

Add these definitions in `src/Watchslinger.cpp` before `handleButtonPress()`:

```cpp
void Watchslinger::openApp(const WatchslingerAppDescriptor *descriptor) {
  if (descriptor == nullptr || !descriptor->isLaunchable()) {
    return;
  }
  activeApp_ = descriptor->app;
  guiState = APP_STATE;
  activeApp_->onOpen(*this);
}

void Watchslinger::closeActiveApp() {
  WatchslingerApp *closingApp = activeApp_;
  activeApp_ = nullptr;
  if (closingApp != nullptr) {
    closingApp->onClose(*this);
  }
}

bool Watchslinger::dispatchActiveAppButton(WatchslingerButton button) {
  if (activeApp_ == nullptr) {
    return false;
  }
  if (activeApp_->onButton(*this, button)) {
    return true;
  }
  if (button == WatchslingerButton::Back) {
    closeActiveApp();
    showMenu(menuIndex, false);
    return true;
  }
  return false;
}
```

- [ ] **Step 6: Run registry and custom app verification**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
pio ci examples/Watchslinger/CustomApp/CustomApp.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: host registry test passes and the custom app example compiles.

- [ ] **Step 7: Commit**

```bash
git add src/Watchslinger.h src/Watchslinger.cpp examples/Watchslinger/CustomApp/CustomApp.ino examples/Watchslinger/CustomApp/settings.h
git commit -m "Add Watchslinger app registry API"
```

## Task 5: Add Registry-Backed Menu Renderer

**Files:**
- Create: `src/WatchslingerMenu.h`
- Create: `src/WatchslingerMenu.cpp`
- Modify: `src/Watchslinger.h`
- Modify: `src/Watchslinger.cpp`

- [ ] **Step 1: Verify hardcoded menu code is still present**

Run:

```bash
rg "const char \\*menuItems|switch \\(menuIndex\\)|MENU_LENGTH" src/Watchslinger.cpp src/config.h
```

Expected: matches appear in `src/Watchslinger.cpp` and `src/config.h`.

- [ ] **Step 2: Add the menu renderer header**

Create `src/WatchslingerMenu.h`:

```cpp
#ifndef WATCHSLINGER_MENU_H
#define WATCHSLINGER_MENU_H

#include <stdint.h>
#include "WatchslingerApp.h"

class Watchslinger;

class WatchslingerMenu {
public:
  WatchslingerMenu(Watchslinger &watch, WatchslingerAppRegistry registry)
      : watch_(watch), registry_(registry) {}

  void show(uint8_t selectedIndex, bool partialRefresh, bool markEntered);
  uint8_t wrappedIndex(uint8_t selectedIndex, int8_t delta) const;
  void launch(uint8_t selectedIndex);

private:
  void showEmpty(bool partialRefresh);

  Watchslinger &watch_;
  WatchslingerAppRegistry registry_;
};

#endif
```

- [ ] **Step 3: Add the menu renderer implementation**

Create `src/WatchslingerMenu.cpp`:

```cpp
#include "WatchslingerMenu.h"
#include "Watchslinger.h"

#include <Fonts/FreeMonoBold9pt7b.h>

void WatchslingerMenu::show(uint8_t selectedIndex, bool partialRefresh,
                            bool markEntered) {
  if (registry_.count() == 0) {
    showEmpty(partialRefresh);
    return;
  }

  selectedIndex = wrappedIndex(selectedIndex, 0);
  menuIndex = selectedIndex;

  watch_.display.setFullWindow();
  watch_.display.fillScreen(GxEPD_BLACK);
  watch_.display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;

  for (uint8_t i = 0; i < registry_.count(); i++) {
    const WatchslingerAppDescriptor *descriptor = registry_.get(i);
    const char *label = descriptor != nullptr && descriptor->label != nullptr
                            ? descriptor->label
                            : "";
    int16_t yPos = MENU_HEIGHT + (MENU_HEIGHT * i);
    watch_.display.setCursor(0, yPos);
    if (i == selectedIndex) {
      watch_.display.getTextBounds(label, 0, yPos, &x1, &y1, &w, &h);
      watch_.display.fillRect(x1 - 1, y1 - 10, DISPLAY_WIDTH, h + 15,
                              GxEPD_WHITE);
      watch_.display.setTextColor(GxEPD_BLACK);
      watch_.display.println(label);
    } else {
      watch_.display.setTextColor(GxEPD_WHITE);
      watch_.display.println(label);
    }
  }

  watch_.display.display(partialRefresh);
  guiState = MAIN_MENU_STATE;
  if (markEntered) {
    alreadyInMenu = false;
  }
}

uint8_t WatchslingerMenu::wrappedIndex(uint8_t selectedIndex,
                                       int8_t delta) const {
  uint8_t count = registry_.count();
  if (count == 0) {
    return 0;
  }
  int16_t next = selectedIndex + delta;
  while (next < 0) {
    next += count;
  }
  while (next >= count) {
    next -= count;
  }
  return static_cast<uint8_t>(next);
}

void WatchslingerMenu::launch(uint8_t selectedIndex) {
  const WatchslingerAppDescriptor *descriptor =
      registry_.get(wrappedIndex(selectedIndex, 0));
  watch_.openApp(descriptor);
}

void WatchslingerMenu::showEmpty(bool partialRefresh) {
  watch_.display.setFullWindow();
  watch_.display.fillScreen(GxEPD_BLACK);
  watch_.display.setFont(&FreeMonoBold9pt7b);
  watch_.display.setTextColor(GxEPD_WHITE);
  watch_.display.setCursor(45, 100);
  watch_.display.println("No apps");
  watch_.display.display(partialRefresh);
  menuIndex = 0;
  guiState = MAIN_MENU_STATE;
  alreadyInMenu = false;
}
```

- [ ] **Step 4: Include the menu header and replace menu rendering methods**

Add this include to `src/Watchslinger.cpp`:

```cpp
#include "WatchslingerMenu.h"
```

Add this extern declaration in `src/Watchslinger.h` with the other `RTC_DATA_ATTR` globals:

```cpp
extern RTC_DATA_ATTR bool alreadyInMenu;
```

Replace `Watchslinger::showMenu` and `Watchslinger::showFastMenu` with:

```cpp
void Watchslinger::showMenu(byte menuIndex, bool partialRefresh) {
  WatchslingerMenu menu(*this, appRegistry());
  menu.show(menuIndex, partialRefresh, true);
}

void Watchslinger::showFastMenu(byte menuIndex) {
  WatchslingerMenu menu(*this, appRegistry());
  menu.show(menuIndex, true, false);
}
```

- [ ] **Step 5: Verify hardcoded menu labels are gone from the renderer path**

Run:

```bash
rg "const char \\*menuItems" src/Watchslinger.cpp src/WatchslingerMenu.cpp
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
pio ci examples/WatchFaces/7_SEG/7_SEG.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: `rg` prints no matches. Host registry test passes. `7_SEG.ino` compiles.

- [ ] **Step 6: Commit**

```bash
git add src/WatchslingerMenu.h src/WatchslingerMenu.cpp src/Watchslinger.h src/Watchslinger.cpp
git commit -m "Render menu from app registry"
```

## Task 6: Route Buttons Through Menu And App Lifecycle

**Files:**
- Modify: `src/Watchslinger.h`
- Modify: `src/Watchslinger.cpp`

- [ ] **Step 1: Verify duplicated menu dispatch is still present**

Run:

```bash
rg "switch \\(menuIndex\\)|MENU_LENGTH" src/Watchslinger.cpp
```

Expected: matches appear in `handleButtonPress()`.

- [ ] **Step 2: Add menu selection helpers to `Watchslinger.h`**

Add these private method declarations before existing private helpers:

```cpp
  void _selectMenuItem();
  void _moveMenuSelection(int8_t delta, bool fastRefresh);
  void _returnToWatchFace();
```

- [ ] **Step 3: Implement menu selection helpers**

Add these definitions before `Watchslinger::handleButtonPress()`:

```cpp
void Watchslinger::_selectMenuItem() {
  WatchslingerMenu menu(*this, appRegistry());
  menu.launch(menuIndex);
}

void Watchslinger::_moveMenuSelection(int8_t delta, bool fastRefresh) {
  WatchslingerMenu menu(*this, appRegistry());
  menuIndex = menu.wrappedIndex(menuIndex, delta);
  if (fastRefresh) {
    showFastMenu(menuIndex);
  } else {
    showMenu(menuIndex, true);
  }
}

void Watchslinger::_returnToWatchFace() {
  RTC.read(currentTime);
  showWatchFace(false);
}
```

- [ ] **Step 4: Replace initial wake button dispatch**

In `Watchslinger::handleButtonPress()`, replace the first menu/back/up/down dispatch block before the fast-menu loop with:

```cpp
  if (wakeupBit & MENU_BTN_MASK) {
    if (guiState == WATCHFACE_STATE) {
      showMenu(menuIndex, false);
    } else if (guiState == MAIN_MENU_STATE) {
      _selectMenuItem();
    } else if (guiState == APP_STATE) {
      dispatchActiveAppButton(WatchslingerButton::Menu);
    }
  } else if (wakeupBit & BACK_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) {
      _returnToWatchFace();
    } else if (guiState == APP_STATE) {
      if (!dispatchActiveAppButton(WatchslingerButton::Back)) {
        showMenu(menuIndex, false);
      }
    } else if (guiState == FW_UPDATE_STATE) {
      showMenu(menuIndex, false);
    }
  } else if (wakeupBit & UP_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) {
      _moveMenuSelection(-1, false);
    } else if (guiState == APP_STATE) {
      dispatchActiveAppButton(WatchslingerButton::Up);
    }
  } else if (wakeupBit & DOWN_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) {
      _moveMenuSelection(1, false);
    } else if (guiState == APP_STATE) {
      dispatchActiveAppButton(WatchslingerButton::Down);
    }
  }
```

- [ ] **Step 5: Replace fast-menu button dispatch**

Inside the fast-menu `while (!timeout)` loop, replace the four button branches with:

```cpp
      if (digitalRead(MENU_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) {
          _selectMenuItem();
        } else if (guiState == APP_STATE) {
          dispatchActiveAppButton(WatchslingerButton::Menu);
        }
      } else if (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) {
          _returnToWatchFace();
          break;
        } else if (guiState == APP_STATE) {
          if (!dispatchActiveAppButton(WatchslingerButton::Back)) {
            showMenu(menuIndex, false);
          }
        } else if (guiState == FW_UPDATE_STATE) {
          showMenu(menuIndex, false);
        }
      } else if (digitalRead(UP_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) {
          _moveMenuSelection(-1, true);
        } else if (guiState == APP_STATE) {
          dispatchActiveAppButton(WatchslingerButton::Up);
        }
      } else if (digitalRead(DOWN_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) {
          _moveMenuSelection(1, true);
        } else if (guiState == APP_STATE) {
          dispatchActiveAppButton(WatchslingerButton::Down);
        }
      }
```

- [ ] **Step 6: Verify duplicated dispatch and `MENU_LENGTH` are gone from implementation**

Run:

```bash
rg "switch \\(menuIndex\\)|MENU_LENGTH" src/Watchslinger.cpp
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
pio ci examples/WatchFaces/7_SEG/7_SEG.ino --lib . --project-conf platformio.ini -e watchy_v3
pio ci examples/Watchslinger/CustomApp/CustomApp.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: `rg` prints no matches. Host registry test passes. Both PlatformIO compile smokes pass.

- [ ] **Step 7: Commit**

```bash
git add src/Watchslinger.h src/Watchslinger.cpp
git commit -m "Route menu buttons through app launcher"
```

## Task 7: Final Verification And Documentation Check

**Files:**
- Modify only if verification exposes a concrete issue in files touched by earlier tasks.

- [ ] **Step 1: Run complete host checks**

Run:

```bash
g++ -std=c++11 -Isrc test/host/test_app_registry.cpp -o /tmp/watchslinger_app_registry_test && /tmp/watchslinger_app_registry_test
g++ -std=c++11 -Isrc test/host/test_v3_macro.cpp -o /tmp/watchslinger_v3_macro_test && /tmp/watchslinger_v3_macro_test
```

Expected: registry test prints `all app registry tests passed`; macro test exits with status 0.

- [ ] **Step 2: Run PlatformIO compile smokes**

Run:

```bash
pio ci examples/WatchFaces/Basic/Basic.ino --lib . --project-conf platformio.ini -e watchy_v3
pio ci examples/WatchFaces/7_SEG/7_SEG.ino --lib . --project-conf platformio.ini -e watchy_v3
pio ci examples/Watchslinger/Canonical/Canonical.ino --lib . --project-conf platformio.ini -e watchy_v3
pio ci examples/Watchslinger/CustomApp/CustomApp.ino --lib . --project-conf platformio.ini -e watchy_v3
```

Expected: all four examples compile for `watchy_v3`.

- [ ] **Step 3: Verify compatibility and scope**

Run:

```bash
rg "using Watchy = Watchslinger" src/Watchy.h
rg "class Watchslinger" src/Watchslinger.h
rg "class Watchy" src
rg "ARDUINO_ESP32S3_DEV" src examples/WatchFaces/7_SEG
git status --short
```

Expected:

```text
src/Watchy.h:using Watchy = Watchslinger;
src/Watchslinger.h:class Watchslinger {
```

`rg "class Watchy" src` prints no matches. `rg "ARDUINO_ESP32S3_DEV" ...` prints no matches. `git status --short` is clean or lists only intentional fixes made during this task.

- [ ] **Step 4: Inspect branch commits**

Run:

```bash
git log --oneline --decorate --max-count=12
```

Expected: implementation commits are on `watchslinger-foundation`, after the approved design spec and worktree ignore commits.

- [ ] **Step 5: Commit any verification fixes**

If Step 1, Step 2, or Step 3 required a focused fix, commit it with:

```bash
git add src examples test platformio.ini
git commit -m "Verify Watchslinger foundation"
```

If no files changed, skip this commit.
