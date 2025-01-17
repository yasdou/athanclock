#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
#include "display.h"
#include "audio.h"
#include "api.h"
#include <TimeLib.h>


// Display-Pins definieren (anpassen an dein Setup)
#define TFT_CS D8
#define TFT_RST D4
#define TFT_DC D3


Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);

void showBootMessage(const char* message) {
  // Bildschirm aktualisieren mit dem aktuellen Status
  display.fillRect(0, 70, 128, 50, ST77XX_WHITE);  // Bereich löschen
  display.setCursor(10, 80);
  display.setTextSize(1);  // Normale Textgröße
  display.setTextColor(ST77XX_BLACK);
  display.print(message);
}

// NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);  // Zeitzone UTC+1 (Deutschland)

// Variablen für das Datum
int currentDay, currentMonth, currentYear;
String apiUrl;  // Die dynamische URL

// Variablen für Gebetszeiten
String fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime;
unsigned long lastPrayerUpdate = 0;  // Zeit der letzten Gebetszeiten-Aktualisierung

bool fajrReminderPlayed = false;
bool shurukReminderPlayed = false;
bool dhuhrReminderPlayed = false;
bool asrReminderPlayed = false;
bool maghribReminderPlayed = false;
bool ishaReminderPlayed = false;

int lastUpdatedMinute = -1;  // Speichert die zuletzt aktualisierte Minute

void setup() {
  Serial.begin(115200);

  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_WHITE);

  // Begrüßungstext für "Athan Clock" 
  display.setFont(NULL); // Standardschriftart
  display.setTextColor(ST77XX_BLACK);
  display.fillScreen(ST77XX_WHITE);

  display.setTextSize(2);  // Große Schrift für den Titel
  int16_t x1, y1;
  uint16_t w, h;

  // Zentrierung für "Athan"
  display.getTextBounds("Athan", 0, 0, &x1, &y1, &w, &h);  // Berechne Textgröße
  int16_t xAthan = (display.width() - w) / 2;              // Zentriere horizontal
  display.setCursor(xAthan, 20);                           // Vertikale Position für die erste Zeile
  display.print("Athan");

  // Zentrierung für "Clock"
  display.getTextBounds("Clock", 0, 0, &x1, &y1, &w, &h);  // Berechne Textgröße
  int16_t xClock = (display.width() - w) / 2;              // Zentriere horizontal
  display.setCursor(xClock, 50);                           // Vertikale Position für die zweite Zeile
  display.print("Clock");

  // DFPlayer Mini initialisieren
  showBootMessage("DFPlayer init...");
  setupAudio();

  // Boot-Vorgang anzeigen
  showBootMessage("WLAN verbinden...");
  // Mit WiFi verbinden
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Mit WLAN verbunden!");
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.localIP());
  showBootMessage("WLAN verbunden!");

  // NTP-Client starten
  timeClient.begin();

  // Aktuelles Datum ermitteln
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();  // Zeit in Sekunden seit 1970
  currentDay = day(epochTime);                   // Extrahiere den Tag
  currentMonth = month(epochTime);               // Extrahiere den Monat
  currentYear = year(epochTime);                 // Extrahiere das Jahr

  // Dynamische URL erstellen
  apiUrl = "http://api.aladhan.com/v1/timingsByCity/" + String(currentDay) + "-" + String(currentMonth) + "-" + String(currentYear) + "?city=Mainz&country=Germany&method=2";

  // API URL ausgeben
  Serial.println("Dynamische API-URL: ");
  Serial.println(apiUrl);

  // Gebetszeiten abrufen
  showBootMessage("Zeiten abrufen...");
  fetchPrayerTimes(fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, apiUrl);
  showBootMessage("Zeiten Abruf erfolgreich!");
  delay(1000);          // Fertig

  // Athan abspielen direkt nach dem Start
  delay(1000);                       // Fertig
  display.fillScreen(ST77XX_WHITE);  // Bildschirm leeren
  display.setCursor(10, 10);
  display.setTextSize(1);
  showBootMessage("Boot abgeschlossen!");
  Serial.println("Spiele Boot Ton ab...");
  playBoot();
  // // Warte, bis die Audio-Wiedergabe abgeschlossen ist
  // while (!myDFPlayer.available() || myDFPlayer.readType() != DFPlayerPlayFinished) {
  //     delay(100); // Kurz warten, um die CPU nicht zu blockieren
  // }

}

bool isTimeForReminder(String prayerTime, bool& reminderPlayed) {
  String currentTime = timeClient.getFormattedTime().substring(0, 5);  // Nur HH:MM vergleichen

  // Reminder-Zeit berechnen (15 Minuten vorher)
  int prayerHour = prayerTime.substring(0, 2).toInt();
  int prayerMinute = prayerTime.substring(3, 5).toInt();

  int reminderHour = prayerHour;
  int reminderMinute = prayerMinute - 15;

  if (reminderMinute < 0) {
    reminderMinute += 60;
    reminderHour -= 1;
    if (reminderHour < 0) {
      reminderHour = 23;
    }
  }

  String reminderTime = (reminderHour < 10 ? "0" : "") + String(reminderHour) + ":" + (reminderMinute < 10 ? "0" : "") + String(reminderMinute);

  if (currentTime == reminderTime && !reminderPlayed) {
    Serial.print("Reminder ausgelöst für: ");
    Serial.println(reminderTime);
    reminderPlayed = true;  // Verhindere mehrfaches Abspielen
    return true;
  }

  if (currentTime != reminderTime) {
    if (reminderPlayed) {
      Serial.print("Reminder zurückgesetzt für: ");
      Serial.println(prayerTime);
    }
    reminderPlayed = false;  // Rücksetzen
  }

  return false;
}


bool shouldPlayAthan(String prayerTime) {
  String currentTime = timeClient.getFormattedTime().substring(0, 5);  // Nur HH:MM vergleichen
  return (currentTime == prayerTime);
}

void loop() {
  // NTP-Client aktualisieren
  timeClient.update();

  int currentMinute = timeClient.getMinutes();
  if (currentMinute != lastUpdatedMinute) {
    // Display nur einmal pro Minute aktualisieren
    updateDisplay(display, fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, timeClient.getFormattedTime());
    lastUpdatedMinute = currentMinute;
  }

  // Gebetszeiten um Mitternacht aktualisieren
  if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0 && millis() - lastPrayerUpdate > 60 * 1000) {
    fetchPrayerTimes(fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, apiUrl);
    lastPrayerUpdate = millis();
    Serial.println("Gebetszeiten um Mitternacht aktualisiert.");
  }

  // Anzeige des Gebets-Countdowns 15 Minuten vorher
  if (isTimeForReminder(fajrTime, fajrReminderPlayed)) {
    Serial.println("Reminder-Check läuft...");
    showPrayerReminder(display, "Fajr", fajrTime);
  }
  if (isTimeForReminder(shurukTime, shurukReminderPlayed)) {
    Serial.println("Reminder-Check läuft...");
    showPrayerReminder(display, "Shuruk", shurukTime);
  }
  if (isTimeForReminder(dhuhrTime, dhuhrReminderPlayed)) {
    Serial.println("Reminder-Check läuft...");
    showPrayerReminder(display, "Dhuhr", dhuhrTime);
  }
  if (isTimeForReminder(asrTime, asrReminderPlayed)) {
    Serial.println("Reminder-Check läuft...");
    showPrayerReminder(display, "Asr", asrTime);
  }
  if (isTimeForReminder(maghribTime, maghribReminderPlayed)) {
    Serial.println("Reminder-Check läuft...");
    showPrayerReminder(display, "Maghrib", maghribTime);
  }
  if (isTimeForReminder(ishaTime, ishaReminderPlayed)) {
    Serial.println("Reminder-Check läuft...");
    showPrayerReminder(display, "Isha", ishaTime);
  }

  // Athan abspielen, wenn es Zeit ist
  if (shouldPlayAthan(fajrTime) || shouldPlayAthan(shurukTime) || shouldPlayAthan(dhuhrTime) || shouldPlayAthan(asrTime) || shouldPlayAthan(maghribTime) || shouldPlayAthan(ishaTime)) {
    playAthan();
  }

  delay(1000);  // 1-Sekunden-Intervall
}