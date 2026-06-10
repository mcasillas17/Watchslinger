#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/WeatherCodeMapper.h"

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

int main() {
  WatchslingerWeatherCodeMapping clear = watchslingerMapOpenMeteoCode(0);
  ASSERT_EQ(800, clear.conditionCode);
  ASSERT_TRUE(strcmp("Clear", clear.description) == 0);

  WatchslingerWeatherCodeMapping fog = watchslingerMapOpenMeteoCode(45);
  ASSERT_EQ(741, fog.conditionCode);
  ASSERT_TRUE(strcmp("Fog", fog.description) == 0);

  WatchslingerWeatherCodeMapping drizzle = watchslingerMapOpenMeteoCode(53);
  ASSERT_EQ(300, drizzle.conditionCode);
  ASSERT_TRUE(strcmp("Drizzle", drizzle.description) == 0);

  WatchslingerWeatherCodeMapping rain = watchslingerMapOpenMeteoCode(65);
  ASSERT_EQ(500, rain.conditionCode);
  ASSERT_TRUE(strcmp("Rain", rain.description) == 0);

  WatchslingerWeatherCodeMapping snow = watchslingerMapOpenMeteoCode(75);
  ASSERT_EQ(600, snow.conditionCode);
  ASSERT_TRUE(strcmp("Snow", snow.description) == 0);

  WatchslingerWeatherCodeMapping thunderstorm = watchslingerMapOpenMeteoCode(95);
  ASSERT_EQ(200, thunderstorm.conditionCode);
  ASSERT_TRUE(strcmp("Thunderstorm", thunderstorm.description) == 0);

  WatchslingerWeatherCodeMapping unknown = watchslingerMapOpenMeteoCode(999);
  ASSERT_EQ(800, unknown.conditionCode);
  ASSERT_TRUE(strcmp("Unknown", unknown.description) == 0);

  ASSERT_TRUE(watchslingerIsHttpsUrl("https://api.open-meteo.com/v1/forecast"));
  ASSERT_TRUE(!watchslingerIsHttpsUrl("http://api.openweathermap.org/data/2.5/weather"));
  ASSERT_TRUE(!watchslingerIsHttpsUrl(""));
  ASSERT_TRUE(!watchslingerIsHttpsUrl(nullptr));

  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey(""));
  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey(nullptr));
  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey("YOUR_OPENWEATHERMAP_API_KEY"));
  ASSERT_TRUE(watchslingerIsMissingOpenWeatherMapKey("REPLACE_WITH_YOUR_OPENWEATHERMAP_API_KEY"));
  ASSERT_TRUE(!watchslingerIsMissingOpenWeatherMapKey("user-supplied-key"));

  printf("all weather helper tests passed\n");
  return 0;
}
