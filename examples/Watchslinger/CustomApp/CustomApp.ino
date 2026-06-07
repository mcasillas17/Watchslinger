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
