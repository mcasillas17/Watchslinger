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
