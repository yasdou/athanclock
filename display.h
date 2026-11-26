#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_ST7735.h>

#include "prayer_logic.h"

extern Adafruit_ST7735 display;

void setupDisplay(Adafruit_ST7735& display);

// Startbildschirm: Stadt, Gebetszeiten (Minuten seit Mitternacht, -1 = unbekannt), Uhrzeit
void updateDisplay(Adafruit_ST7735& display, const String& city, const int minutes[prayer::kPrayerCount], int hour,
                   int minute);

// Countdown: einmal den Rahmen zeichnen, danach nur noch die Zahl aktualisieren
// (kein Flackern durch fillScreen() jede Sekunde).
void showReminderScreen(Adafruit_ST7735& display, const char* prayerName);
void updateReminderCountdown(Adafruit_ST7735& display, long secondsLeft);

// Anleitung für den Einrichtungs-Hotspot
void showWifiSetupScreen(Adafruit_ST7735& display, const char* apName, const char* password, bool hasSavedWifi);

// Fortschritt bei Firmware-Updates (percent < 0 = Fehler)
void showUpdateProgress(Adafruit_ST7735& display, int percent);

#endif
