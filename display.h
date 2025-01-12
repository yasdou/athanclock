#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>

// Funktionen
void setupDisplay(Adafruit_SSD1306& display);
void updateDisplay(Adafruit_SSD1306& display, String fajr, String dhuhr, String asr, String maghrib, String isha, String time);

#endif
