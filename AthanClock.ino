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

bool isReminderActive = false; // Gibt an, ob aktuell ein Reminder angezeigt wird
bool isTimeForReminder(String prayerTime, bool& reminderPlayed) {
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

    // Aktuelle Zeit in Minuten berechnen
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentTimeInMinutes = currentHour * 60 + currentMinute;

    // Reminder- und Gebetszeit in Minuten berechnen
    int reminderTimeInMinutes = reminderHour * 60 + reminderMinute;
    int prayerTimeInMinutes = prayerHour * 60 + prayerMinute;

    // Prüfen, ob die aktuelle Zeit im gewünschten Bereich liegt
    if (currentTimeInMinutes >= reminderTimeInMinutes && currentTimeInMinutes < prayerTimeInMinutes) {
        if (!reminderPlayed) {
            Serial.print("Reminder gestartet für: ");
            Serial.println(prayerTime);
            reminderPlayed = true;  // Verhindert mehrfaches Triggern
        }
        return true;
    } else {
        if (reminderPlayed) {
            Serial.print("Reminder zurückgesetzt für: ");
            Serial.println(prayerTime);
        }
        reminderPlayed = false;  // Reminder zurücksetzen, wenn außerhalb des Zeitbereichs
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

   if (!isReminderActive) {
        int currentMinute = timeClient.getMinutes();
        if (currentMinute != lastUpdatedMinute) {
            updateDisplay(display, fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, timeClient.getFormattedTime());
            lastUpdatedMinute = currentMinute;
        }
    }

  // Gebetszeiten um Mitternacht aktualisieren
  if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0 && millis() - lastPrayerUpdate > 60 * 1000) {
    fetchPrayerTimes(fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, apiUrl);
    lastPrayerUpdate = millis();
    Serial.println("Gebetszeiten um Mitternacht aktualisiert.");
  }

  // Reminder für alle Gebetszeiten prüfen
  // Gebetscountdown t -15 min
    if (isTimeForReminder(fajrTime, fajrReminderPlayed)) {
        isReminderActive = true; // Reminder aktiv
        showPrayerReminder(display, "Fajr", fajrTime);
    } else if (isTimeForReminder(shurukTime, shurukReminderPlayed)) {
        isReminderActive = true;
        showPrayerReminder(display, "Shuruk", shurukTime);
    } else if (isTimeForReminder(dhuhrTime, dhuhrReminderPlayed)) {
        isReminderActive = true;
        showPrayerReminder(display, "Dhuhr", dhuhrTime);
    } else if (isTimeForReminder(asrTime, asrReminderPlayed)) {
        isReminderActive = true;
        showPrayerReminder(display, "Asr", asrTime);
    } else if (isTimeForReminder(maghribTime, maghribReminderPlayed)) {
        isReminderActive = true;
        showPrayerReminder(display, "Maghrib", maghribTime);
    } else if (isTimeForReminder(ishaTime, ishaReminderPlayed)) {
        isReminderActive = true;
        showPrayerReminder(display, "Isha", ishaTime);
    } else {
        // Kein Reminder aktiv
        isReminderActive = false;
    }

  // Athan abspielen, wenn es Zeit ist
  if (shouldPlayAthan(fajrTime) || shouldPlayAthan(shurukTime) || shouldPlayAthan(dhuhrTime) || shouldPlayAthan(asrTime) || shouldPlayAthan(maghribTime) || shouldPlayAthan(ishaTime)) {
    playAthan();
  }

  // // Teste den Reminder für Fajr-Gebet mit simulierten Zeiten
  //   testReminder(display, "Fajr", "05:30", 5, 15, 0);  // Simulierte Zeit: 05:15:00
  //   delay(5000);  // Warten, bevor ein neuer Test durchgeführt wird
  //   testReminder(display, "Fajr", "05:30", 5, 20, 0);  // Simulierte Zeit: 05:20:00
  //   delay(5000);  // Warten, bevor ein neuer Test durchgeführt wird
  //   testReminder(display, "Fajr", "05:30", 5, 30, 0);  // Simulierte Zeit: 05:30:00
  //   delay(5000);  // Warten, bevor ein neuer Test durchgeführt wird
  //   testReminder(display, "Fajr", "05:30", 5, 35, 0);  // Simulierte Zeit: 05:35:00
  //   delay(5000);  // Warten, bevor ein neuer Test durchgeführt wird

  delay(1000);  // 1-Sekunden-Intervall
}

// void testReminder(Adafruit_ST7735& display, String prayerName, String prayerTime, int simulatedHour, int simulatedMinute, int simulatedSecond) {
//     Serial.println("== Starte Reminder-Test ==");
    
//     // Gebetszeit extrahieren
//     int prayerHour = prayerTime.substring(0, 2).toInt();
//     int prayerMinute = prayerTime.substring(3, 5).toInt();

//     // Reminder-Zeit berechnen (15 Minuten vor der Gebetszeit)
//     int reminderMinute = prayerMinute - 15;
//     int reminderHour = prayerHour;
//     if (reminderMinute < 0) {
//         reminderMinute += 60;
//         reminderHour -= 1;
//         if (reminderHour < 0) {
//             reminderHour = 23;
//         }
//     }

//     Serial.print("Reminder-Zeit: ");
//     Serial.print(reminderHour);
//     Serial.print(":");
//     Serial.println(reminderMinute);

//     // Simulierte Zeit
//     Serial.print("Simulierte Zeit: ");
//     Serial.print(simulatedHour);
//     Serial.print(":");
//     Serial.print(simulatedMinute);
//     Serial.print(":");
//     Serial.println(simulatedSecond);

//     // Berechnung der verbleibenden Sekunden bis zum Gebet
//     int remainingSeconds = (prayerHour - simulatedHour) * 3600 + (prayerMinute - simulatedMinute) * 60 - simulatedSecond;

//     if (remainingSeconds <= 15 * 60 && remainingSeconds > 0) { 
//         // Countdown berechnen
//         int countdownMinutes = remainingSeconds / 60;
//         int countdownSeconds = remainingSeconds % 60;

//         String countdownText = (countdownMinutes < 10 ? "0" : "") + String(countdownMinutes) + ":" + 
//                                (countdownSeconds < 10 ? "0" : "") + String(countdownSeconds);

//         Serial.print("Countdown bis zum Gebet: ");
//         Serial.println(countdownText);

//         // Reminder anzeigen
//         display.fillScreen(ST77XX_WHITE);
//         display.setTextSize(2);
//         int16_t x1, y1;
//         uint16_t width, height;

//         // Gebetsname anzeigen
//         display.getTextBounds(prayerName, 0, 0, &x1, &y1, &width, &height);
//         display.setCursor((display.width() - width) / 2, 20);
//         display.print(prayerName);

//         // "In" anzeigen
//         display.setTextSize(1);
//         display.setCursor((display.width() - 50) / 2, 50);
//         display.print("in");

//         // Countdown anzeigen
//         display.setTextSize(2);
//         display.getTextBounds(countdownText, 0, 0, &x1, &y1, &width, &height);
//         display.setCursor((display.width() - width) / 2, 70);
//         display.print(countdownText);

//     } else if (remainingSeconds <= 0) {
//         Serial.println("Gebetszeit erreicht oder überschritten.");
//     } else {
//         Serial.println("Reminder noch nicht aktiv.");
//     }
// }


