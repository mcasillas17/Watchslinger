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
