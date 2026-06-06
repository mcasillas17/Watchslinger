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
