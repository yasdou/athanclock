#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <NTPClient.h>
#include <WiFiUdp.h> 
#include <SD.h>
#include "display.h"

// Deklaration der Variablen (extern)
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

void setupDisplay(Adafruit_ST7735& display) {
    display.initR(INITR_BLACKTAB); // Display initialisieren
    display.fillScreen(ST77XX_WHITE); // Hintergrund weiß
    display.setTextColor(ST77XX_BLACK); // Schrift schwarz
    display.setTextWrap(false);
    display.setRotation(0); // Hochformat
}

void updateDisplay(Adafruit_ST7735& display, String fajr, String shuruk, String dhuhr, String asr, String maghrib, String isha, String time) {
    // Nur die ersten fünf Zeichen der Zeit (HH:MM) extrahieren
    String formattedTime = time.substring(0, 5);  
    display.fillScreen(ST77XX_WHITE); // Hintergrund weiß

    int16_t x1, y1;
    uint16_t width, height;

    int lineSpacing = 15; // Abstand zwischen den Zeilen
    int startY = 20; // Startpunkt für die erste Zeile

    // Fajr
    display.setTextSize(1);
    String fajrText = "Fajr: ";
    display.getTextBounds(fajrText + fajr, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, startY);
    display.print(fajrText); display.println(fajr);

    // Shuruk
    String shurukText = "Shuruk: ";
    display.getTextBounds(shurukText + shuruk, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, startY + lineSpacing);
    display.print(shurukText); display.println(shuruk);

    // Dhuhr
    String dhuhrText = "Dhuhr: ";
    display.getTextBounds(dhuhrText + dhuhr, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, startY + 2 * lineSpacing);
    display.print(dhuhrText); display.println(dhuhr);

    // Asr
    String asrText = "Asr: ";
    display.getTextBounds(asrText + asr, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, startY + 3 * lineSpacing);
    display.print(asrText); display.println(asr);

    // Maghrib
    String maghribText = "Maghrib: ";
    display.getTextBounds(maghribText + maghrib, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, startY + 4 * lineSpacing);
    display.print(maghribText); display.println(maghrib);

    // Isha
    String ishaText = "Isha: ";
    display.getTextBounds(ishaText + isha, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, startY + 5 * lineSpacing);
    display.print(ishaText); display.println(isha);

    // Time (HH:MM format) - 35 Pixel unter der letzten Zeile
    display.setTextSize(2);
    int timeY = startY + 5 * lineSpacing + 35; // Y-Position der Uhrzeit
    display.getTextBounds(formattedTime, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2, timeY);
    display.print(formattedTime);
}

// Neuer, sauberer Reminder mit eigenem Countdown-Status
void showPrayerReminder(Adafruit_ST7735& display, String prayerName, unsigned long remainingSeconds) {
    // remainingSeconds kommt fertig berechnet aus athanclock.ino

    int countdownMinutes = remainingSeconds / 60;
    int countdownSeconds = remainingSeconds % 60;

    char countdownText[9];
    sprintf(countdownText, "%02d:%02d", countdownMinutes, countdownSeconds);

    // Nur den unteren Bereich für den Countdown löschen, nicht den ganzen Screen
    // Oben könntest du z.B. den Gebetsnamen einmalig zeichnen
    display.fillRect(0, 60, display.width(), 40, ST77XX_WHITE);

    // Gebetsname (oben, nur Textbereich – wird bei jedem Aufruf neu geschrieben,
    // flackert aber wenig, weil nur kleiner Bereich)
    display.setTextSize(2);
    display.setTextColor(ST77XX_BLACK);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(prayerName, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, 20);
    display.print(prayerName);

    // „in“ Text
    display.setTextSize(1);
    display.getTextBounds("in", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, 45);
    display.print("in");

    // Countdown in groß unten
    display.setTextSize(2);
    display.getTextBounds(countdownText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, 70);
    display.print(countdownText);
}

