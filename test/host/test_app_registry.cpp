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
