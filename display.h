#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 display;

// Funktionen
void setupDisplay(Adafruit_ST7735& display);
void updateDisplay(Adafruit_ST7735& display, String fajr, String shuruk, String dhuhr, String asr, String maghrib, String isha, String time);
void showPrayerReminder(Adafruit_ST7735& display, String prayerName, unsigned long remainingSeconds);

#endif
