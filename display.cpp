#include "display.h"

#include <Adafruit_GFX.h>

static const int kCountdownY = 76;
static const int kCountdownTextSize = 3;

static String buildCityHeader(Adafruit_ST7735& display, const String& city) {
  int16_t x1, y1;
  uint16_t wCity, hCity, wEq, hEq;

  String cityText = " " + city + " ";
  display.setTextSize(1);
  display.setFont(NULL);
  display.getTextBounds(cityText, 0, 0, &x1, &y1, &wCity, &hCity);
  display.getTextBounds("=", 0, 0, &x1, &y1, &wEq, &hEq);

  int availableWidth = display.width() - wCity;
  int eqPerSide = (wEq > 0 && availableWidth > 0) ? (availableWidth / 2) / wEq : 0;

  String equals = "";
  for (int i = 0; i < eqPerSide; i++) equals += "=";
  return equals + cityText + equals;
}

static void printCentered(Adafruit_ST7735& display, const String& text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (display.width() - (int)w) / 2;
  display.setCursor(x < 0 ? 0 : x, y);
  display.print(text);
}

void setupDisplay(Adafruit_ST7735& display) {
  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_WHITE);
  display.setTextColor(ST77XX_BLACK);
  display.setTextWrap(false);
  display.setRotation(0);  // Hochformat
}

void updateDisplay(Adafruit_ST7735& display, const String& city, const int minutes[prayer::kPrayerCount], int hour,
                   int minute) {
  display.fillScreen(ST77XX_WHITE);
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setTextWrap(false);

  printCentered(display, buildCityHeader(display, city), 5);

  const int lineSpacing = 15;
  const int startY = 30;
  for (int i = 0; i < prayer::kPrayerCount; i++) {
    char timeText[6];
    prayer::formatHHMM(minutes[i], timeText, sizeof(timeText));
    printCentered(display, String(prayer::kNames[i]) + ": " + timeText, startY + i * lineSpacing);
  }

  char clock[6];
  prayer::formatHHMM(hour * 60 + minute, clock, sizeof(clock));
  display.setTextSize(2);
  printCentered(display, clock, startY + 5 * lineSpacing + 35);
}

void showReminderScreen(Adafruit_ST7735& display, const char* prayerName) {
  display.fillScreen(ST77XX_WHITE);
  display.setTextColor(ST77XX_BLACK);
  display.setTextWrap(false);
  display.setTextSize(2);
  printCentered(display, prayerName, 22);
  display.setTextSize(1);
  printCentered(display, "in", 52);
}

void showWifiSetupScreen(Adafruit_ST7735& display, const char* apName, const char* password, bool hasSavedWifi) {
  display.fillScreen(ST77XX_WHITE);
  display.setTextColor(ST77XX_BLACK);
  display.setTextWrap(false);
  display.setTextSize(1);

  printCentered(display, hasSavedWifi ? "WLAN nicht erreichbar" : "WLAN einrichten", 6);
  display.drawFastHLine(8, 18, display.width() - 16, ST77XX_BLACK);

  display.setCursor(4, 26);
  display.print("1. Verbinden mit:");
  printCentered(display, apName, 40);
  display.setCursor(4, 56);
  display.print("Passwort:");
  display.setTextSize(2);
  printCentered(display, password, 68);
  display.setTextSize(1);

  display.setCursor(4, 92);
  display.print("2. Seite oeffnet von");
  display.setCursor(4, 104);
  display.print("   selbst, sonst:");
  printCentered(display, "192.168.4.1", 116);
  if (hasSavedWifi) {
    printCentered(display, "(versuche es weiter)", 140);
  }
}

void showUpdateProgress(Adafruit_ST7735& display, int percent) {
  static int lastPercent = -2;
  if (percent == lastPercent) return;
  if (lastPercent == -2 || percent == 0) {
    display.fillScreen(ST77XX_WHITE);
    display.setTextColor(ST77XX_BLACK);
    display.setTextSize(1);
    printCentered(display, "Firmware-Update", 30);
    printCentered(display, "Nicht ausschalten!", 45);
    display.drawRect(10, 70, display.width() - 20, 14, ST77XX_BLACK);
  }
  lastPercent = percent;
  if (percent < 0) {
    display.fillRect(0, 95, display.width(), 10, ST77XX_WHITE);
    printCentered(display, "Fehler beim Update", 95);
    lastPercent = -2;
    return;
  }
  int w = (display.width() - 24) * percent / 100;
  display.fillRect(12, 72, w, 10, ST77XX_BLACK);
  char text[8];
  snprintf(text, sizeof(text), "%d %%", percent);
  display.fillRect(0, 95, display.width(), 10, ST77XX_WHITE);
  printCentered(display, text, 95);
}

void updateReminderCountdown(Adafruit_ST7735& display, long secondsLeft) {
  char text[6];
  prayer::formatCountdown(secondsLeft, text, sizeof(text));
  // Nur den Bereich der Zahl übermalen
  display.fillRect(0, kCountdownY, display.width(), 8 * kCountdownTextSize, ST77XX_WHITE);
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(kCountdownTextSize);
  printCentered(display, text, kCountdownY);
}
