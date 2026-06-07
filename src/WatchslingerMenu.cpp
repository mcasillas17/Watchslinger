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
