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
#define TFT_CS    D8
#define TFT_RST   D4
#define TFT_DC    D3

Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);

// NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Zeitzone UTC+1 (Deutschland)

// Variablen für das Datum
int currentDay, currentMonth, currentYear;
String apiUrl; // Die dynamische URL

// Variablen für Gebetszeiten
String fajrTime, dhuhrTime, asrTime, maghribTime, ishaTime;
unsigned long lastPrayerUpdate = 0; // Zeit der letzten Gebetszeiten-Aktualisierung

void setup() {
  Serial.begin(115200);

  // Display initialisieren
  setupDisplay(display);

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

  // NTP-Client starten
  timeClient.begin();

  // Aktuelles Datum ermitteln
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();  // Zeit in Sekunden seit 1970
  currentDay = day(epochTime);        // Extrahiere den Tag
  currentMonth = month(epochTime);    // Extrahiere den Monat
  currentYear = year(epochTime);      // Extrahiere das Jahr

  // Dynamische URL erstellen
  apiUrl = "http://api.aladhan.com/v1/timingsByCity/" + String(currentDay) + "-" + String(currentMonth) + "-" + String(currentYear) + "?city=Mainz&country=Germany&method=2";
  
  // API URL ausgeben
  Serial.println("Dynamische API-URL: ");
  Serial.println(apiUrl);

  // Gebetszeiten abrufen
  fetchPrayerTimes(fajrTime, dhuhrTime, asrTime, maghribTime, ishaTime, apiUrl);
}

bool shouldPlayAthan() {
  String currentTime = timeClient.getFormattedTime().substring(0, 5); // Nur HH:MM vergleichen
  return (currentTime == fajrTime || currentTime == dhuhrTime || currentTime == asrTime || 
          currentTime == maghribTime || currentTime == ishaTime);
}

void loop() {
  // NTP-Client aktualisieren
  timeClient.update();

  // Display aktualisieren
  updateDisplay(display, fajrTime, dhuhrTime, asrTime, maghribTime, ishaTime, timeClient.getFormattedTime());

  // Gebetszeiten um Mitternacht aktualisieren
  if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0 && millis() - lastPrayerUpdate > 60 * 1000) {
    fetchPrayerTimes(fajrTime, dhuhrTime, asrTime, maghribTime, ishaTime, apiUrl);
    lastPrayerUpdate = millis();
    Serial.println("Gebetszeiten um Mitternacht aktualisiert.");
  }

  // Athan abspielen, wenn es Zeit ist
  if (shouldPlayAthan()) {
    playAthan();
  }

  // Aktuelle Uhrzeit alle 1 Sekunde ausgeben
  Serial.print("Aktuelle Uhrzeit: ");
  Serial.println(timeClient.getFormattedTime());

  delay(10000); // 1-Sekunden-Intervall
}
