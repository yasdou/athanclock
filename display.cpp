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

void showPrayerReminder(Adafruit_ST7735& display, String prayerName, String prayerTime) {
    Serial.println("Starte showPrayerReminder...");

    // Gebetszeit extrahieren
    int prayerHour = prayerTime.substring(0, 2).toInt();
    int prayerMinute = prayerTime.substring(3, 5).toInt();

    // Reminder-Zeit berechnen (15 Minuten vor der Gebetszeit)
    int reminderMinute = prayerMinute - 15;
    int reminderHour = prayerHour;
    if (reminderMinute < 0) {
        reminderMinute += 60;
        reminderHour -= 1;
        if (reminderHour < 0) {
            reminderHour = 23;
        }
    }

    // Aktuelle Zeit abrufen
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();

    Serial.print("Aktuelle Zeit: ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.print(currentMinute);
    Serial.print(":");
    Serial.println(currentSecond);

    // Berechnung der verbleibenden Sekunden bis zur Gebetszeit
    int remainingSeconds = (prayerHour - currentHour) * 3600 + (prayerMinute - currentMinute) * 60 - currentSecond;

    // Prüfen, ob sich die aktuelle Zeit im Reminder-Fenster befindet
    int reminderStartSeconds = (reminderHour - currentHour) * 3600 + (reminderMinute - currentMinute) * 60 - currentSecond;

    if (reminderStartSeconds <= 0 && remainingSeconds > 0) {
        // Countdown berechnen
        int countdownMinutes = remainingSeconds / 60;
        int countdownSeconds = remainingSeconds % 60;

        String countdownText = (countdownMinutes < 10 ? "0" : "") + String(countdownMinutes) + ":" + 
                               (countdownSeconds < 10 ? "0" : "") + String(countdownSeconds);

        // Display löschen und Reminder anzeigen
        display.fillScreen(ST77XX_WHITE);
        display.setTextSize(2);
        int16_t x1, y1;
        uint16_t width, height;

        // Gebetsname anzeigen
        display.getTextBounds(prayerName, 0, 0, &x1, &y1, &width, &height);
        display.setCursor((display.width() - width) / 2, 20);
        display.print(prayerName);

        // "In" anzeigen
        display.setTextSize(1);
        display.setCursor((display.width() - 50) / 2, 50);
        display.print("in");

        // Countdown anzeigen
        display.setTextSize(2);
        display.getTextBounds(countdownText, 0, 0, &x1, &y1, &width, &height);
        display.setCursor((display.width() - width) / 2, 70);
        display.print(countdownText);

        Serial.print("Countdown bis zum Gebet: ");
        Serial.println(countdownText);

    } else if (remainingSeconds <= 0) {
        Serial.println("Gebetszeit erreicht oder überschritten.");
    } else {
        Serial.println("Reminder noch nicht aktiv.");
    }
}

