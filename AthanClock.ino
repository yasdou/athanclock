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
#include <time.h>


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

// Countdown-Status für Reminder
bool countdownActive = false;
String countdownPrayerName = "";
unsigned long countdownEndMillis = 0;      // Zeitpunkt, wann das Gebet beginnt
unsigned long lastCountdownUpdate = 0;     // letzte Aktualisierung des Displays
int lastCountdownSecondsShown = -1;        // letztes angezeigtes Sekunden-Ergebnis

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

  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();  // Sekunden seit 1970 (Unix time)

  time_t rawTime = (time_t)epochTime;
  struct tm *ptm = gmtime(&rawTime);   // UTC-Struktur aus epochTime

  currentDay   = ptm->tm_mday;         // 1–31
  currentMonth = ptm->tm_mon + 1;      // 0–11 -> +1 für 1–12
  currentYear  = ptm->tm_year + 1900;  // Jahre seit 1900

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

// Startet den 15-Minuten-Countdown bis zu einem Gebet
void startPrayerCountdown(String prayerName, String prayerTime) {
    int prayerHour = prayerTime.substring(0, 2).toInt();
    int prayerMinute = prayerTime.substring(3, 5).toInt();

    // Zeitpunkt des Gebets heute in Sekunden seit Mitternacht
    unsigned long prayerSeconds = prayerHour * 3600UL + prayerMinute * 60UL;

    // aktuelle Zeit aus timeClient
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();
    unsigned long nowSeconds = currentHour * 3600UL + currentMinute * 60UL + currentSecond;

    // Restsekunden bis zum Gebet
    long remainingSeconds = (long)prayerSeconds - (long)nowSeconds;

    if (remainingSeconds <= 0) {
        // schon vorbei oder genau jetzt, kein Countdown
        countdownActive = false;
        return;
    }

    // 15-Minuten-Fenster: wir starten nur, wenn <= 15 Minuten übrig sind
    if (remainingSeconds > 15 * 60) {
        countdownActive = false;
        return;
    }

    display.fillScreen(ST77XX_WHITE);
    countdownPrayerName = prayerName;
    countdownEndMillis = millis() + (unsigned long)remainingSeconds * 1000UL;
    lastCountdownUpdate = 0;
    lastCountdownSecondsShown = -1;
    countdownActive = true;

    Serial.print("Countdown gestartet für ");
    Serial.print(prayerName);
    Serial.print(" (");
    Serial.print(remainingSeconds);
    Serial.println(" Sekunden bis zum Gebet)");
}

void updatePrayerCountdown() {
    if (!countdownActive) return;

    unsigned long now = millis();
    // nur alle 500ms updaten
    if (now - lastCountdownUpdate < 500) return;
    lastCountdownUpdate = now;

    // verbleibende Millisekunden
    long remainingMillis = (long)countdownEndMillis - (long)now;
    if (remainingMillis <= 0) {
        countdownActive = false;
        isReminderActive = false;  // ← NEU: Reminder auch deaktivieren
        Serial.print("Countdown fertig für ");
        Serial.println(countdownPrayerName);
        return;
    }

    int remainingSeconds = remainingMillis / 1000;
    if (remainingSeconds == lastCountdownSecondsShown) {
        // Sekundenwert hat sich nicht geändert → Display nicht anfassen
        return;
    }
    lastCountdownSecondsShown = remainingSeconds;

    // Reminder-Screen zeichnen (nur Teilbereiche, kein kompletter clearScreen im Loop)
    showPrayerReminder(display, countdownPrayerName, (unsigned long)remainingSeconds);
}

void loop() {
  // NTP-Client aktualisieren
  timeClient.update();

  // Normale Ansicht nur wenn KEIN Reminder aktiv
  if (!isReminderActive && !countdownActive) {
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

  // Reminder-Fenster prüfen und ggf. Countdown starten
  if (isTimeForReminder(fajrTime, fajrReminderPlayed)) {
      if (!countdownActive || countdownPrayerName != "Fajr") {
          startPrayerCountdown("Fajr", fajrTime);
      }
      isReminderActive = true;
  } else if (isTimeForReminder(shurukTime, shurukReminderPlayed)) {
      if (!countdownActive || countdownPrayerName != "Shuruk") {
          startPrayerCountdown("Shuruk", shurukTime);
      }
      isReminderActive = true;
  } else if (isTimeForReminder(dhuhrTime, dhuhrReminderPlayed)) {
      if (!countdownActive || countdownPrayerName != "Dhuhr") {
          startPrayerCountdown("Dhuhr", dhuhrTime);
      }
      isReminderActive = true;
  } else if (isTimeForReminder(asrTime, asrReminderPlayed)) {
      if (!countdownActive || countdownPrayerName != "Asr") {
          startPrayerCountdown("Asr", asrTime);
      }
      isReminderActive = true;
  } else if (isTimeForReminder(maghribTime, maghribReminderPlayed)) {
      if (!countdownActive || countdownPrayerName != "Maghrib") {
          startPrayerCountdown("Maghrib", maghribTime);
      }
      isReminderActive = true;
  } else if (isTimeForReminder(ishaTime, ishaReminderPlayed)) {
      if (!countdownActive || countdownPrayerName != "Isha") {
          startPrayerCountdown("Isha", ishaTime);
      }
      isReminderActive = true;
  } else {
      isReminderActive = false;
      countdownActive = false; // falls Fenster vorbei ist
  }

  // Countdown-Anzeige aktualisieren (wenn aktiv)
  updatePrayerCountdown();


  // Athan abspielen, wenn es Zeit ist
  if (shouldPlayAthan(fajrTime) || shouldPlayAthan(shurukTime) || shouldPlayAthan(dhuhrTime) || shouldPlayAthan(asrTime) || shouldPlayAthan(maghribTime) || shouldPlayAthan(ishaTime)) {
    playAthan();
  }

  delay(1000);  // 1-Sekunden-Intervall
}



