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
