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
  virtual void onClose(Watchslinger &watch) { (void)watch; }
  virtual bool closeAfterOpen() const { return false; }
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
      : items(appItems), count(appItems == nullptr ? 0 : appCount) {}
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

  void onOpen(Watchslinger &watch) override {
    if (handler_ != nullptr) {
      (watch.*handler_)();
    }
  }
  bool closeAfterOpen() const override { return true; }

private:
  WatchslingerLegacyHandler handler_;
};

#endif
