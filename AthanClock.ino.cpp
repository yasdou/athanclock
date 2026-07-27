# 1 "C:\\Users\\yassin\\AppData\\Local\\Temp\\tmpil1i077e"
#include <Arduino.h>
# 1 "C:/Users/yassin/Documents/PlatformIO/Projects/260112-132505-nodemcu/src/AthanClock.ino"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
#include "display.h"
#include "audio.h"
#include "api.h"
#include "html.h"
#include <time.h>
#include "buttons.h"
#include "menu.h"

#define TFT_CS D8
#define TFT_RST D4
#define TFT_DC D3

Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);
void showBootMessage(const char* message);
void updateCurrentDateFromNTP();
void printCurrentDateDebug();
String buildMonthlyApiUrl(int year, int month);
int timeToMinutes(const String& t);
String getPrayerTimeByName(int dayIndex, const String& prayerName);
void printTodayCacheDebug();
String getNextPrayerTimeFor(const String& prayerName);
void refreshDisplayedPrayerTimes();
String getTodayPrayerTimeFor(const String& prayerName);
bool isTimeForReminder(String prayerTime, bool& reminderPlayed, int reminderMode);
bool shouldPlayAthan(String prayerTime, bool& athanPlayed, int athanMode);
void startPrayerCountdown(String prayerName, String prayerTime);
void updatePrayerCountdown();
void printDisplayTimesDebug();
void setup();
void loop();
#line 23 "C:/Users/yassin/Documents/PlatformIO/Projects/260112-132505-nodemcu/src/AthanClock.ino"
void showBootMessage(const char* message) {
  display.fillRect(0, 70, 128, 50, ST77XX_WHITE);
  display.setCursor(10, 80);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.print(message);
}

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000);

PrayerDay monthlyPrayerTimes[31];

int currentDay = 1;
int currentMonth = 1;
int currentYear = 2026;

int loadedMonth = -1;
int loadedYear = -1;
int daysInLoadedMonth = 0;

String apiUrl;

String fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime;
unsigned long lastPrayerUpdate = 0;
unsigned long lastDebugPrint = 0;
unsigned long lastMonthlyLoadAttempt = 0;

bool fajrReminderPlayed = false;
bool shurukReminderPlayed = false;
bool dhuhrReminderPlayed = false;
bool asrReminderPlayed = false;
bool maghribReminderPlayed = false;
bool ishaReminderPlayed = false;

bool fajrAthanPlayed = false;
bool shurukAthanPlayed = false;
bool dhuhrAthanPlayed = false;
bool asrAthanPlayed = false;
bool maghribAthanPlayed = false;
bool ishaAthanPlayed = false;

bool countdownActive = false;
String countdownPrayerName = "";
unsigned long countdownEndMillis = 0;
unsigned long lastCountdownUpdate = 0;
int lastCountdownSecondsShown = -1;

int lastUpdatedMinute = -1;
int lastDisplayDay = -1;

String selectedCity = "Mainz";
int prayerReminderModes[6] = {1,1,1,1,1,1};
int prayerAthanModes[6] = {1,1,1,1,1,1};
String reminderTone = "0";
String athanTone = "0";

IPAddress staticIP(192, 168, 1, 255);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

bool isReminderActive = false;

void updateCurrentDateFromNTP() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  time_t rawTime = (time_t)epochTime;
  struct tm *ptm = gmtime(&rawTime);

  currentDay = ptm->tm_mday;
  currentMonth = ptm->tm_mon + 1;
  currentYear = ptm->tm_year + 1900;
}

void printCurrentDateDebug() {
  Serial.print("Aktuelles Datum laut NTP: ");
  Serial.print(currentDay);
  Serial.print(".");
  Serial.print(currentMonth);
  Serial.print(".");
  Serial.print(currentYear);
  Serial.print(" | Uhrzeit: ");
  Serial.println(timeClient.getFormattedTime());
}

String buildMonthlyApiUrl(int year, int month) {
    return "https://api.aladhan.com/v1/calendarByCity/"
         + String(year) + "/" + String(month)
         + "?city=Mainz&country=Germany&method=2";
}

int timeToMinutes(const String& t) {
  if (t.length() < 5) return -1;
  return t.substring(0, 2).toInt() * 60 + t.substring(3, 5).toInt();
}

String getPrayerTimeByName(int dayIndex, const String& prayerName) {
  if (dayIndex < 0 || dayIndex >= daysInLoadedMonth) {
    return "";
  }

  if (prayerName == "Fajr") return monthlyPrayerTimes[dayIndex].fajr;
  if (prayerName == "Shuruk") return monthlyPrayerTimes[dayIndex].shuruk;
  if (prayerName == "Dhuhr") return monthlyPrayerTimes[dayIndex].dhuhr;
  if (prayerName == "Asr") return monthlyPrayerTimes[dayIndex].asr;
  if (prayerName == "Maghrib") return monthlyPrayerTimes[dayIndex].maghrib;
  if (prayerName == "Isha") return monthlyPrayerTimes[dayIndex].isha;

  return "";
}

void printTodayCacheDebug() {
  int idx = currentDay - 1;

  Serial.print("Heute Index: ");
  Serial.print(idx);
  Serial.print(" | daysInLoadedMonth: ");
  Serial.println(daysInLoadedMonth);

  if (idx >= 0 && idx < daysInLoadedMonth) {
    Serial.print("Cache heute -> Fajr: "); Serial.print(monthlyPrayerTimes[idx].fajr);
    Serial.print(" | Shuruk: "); Serial.print(monthlyPrayerTimes[idx].shuruk);
    Serial.print(" | Dhuhr: "); Serial.print(monthlyPrayerTimes[idx].dhuhr);
    Serial.print(" | Asr: "); Serial.print(monthlyPrayerTimes[idx].asr);
    Serial.print(" | Maghrib: "); Serial.print(monthlyPrayerTimes[idx].maghrib);
    Serial.print(" | Isha: "); Serial.println(monthlyPrayerTimes[idx].isha);
  } else {
    Serial.println("Heutiger Index liegt außerhalb des Monatsarrays.");
  }
}

bool loadMonthlyPrayerTimesIfNeeded(bool forceReload = false) {
  updateCurrentDateFromNTP();

    if (!forceReload && loadedMonth == currentMonth && loadedYear == currentYear && daysInLoadedMonth > 0) {
        return true;
    }

    if (!forceReload && millis() - lastMonthlyLoadAttempt < 30000) {
        Serial.println("Reload übersprungen (Cooldown aktiv).");
        return false;
    }

  lastMonthlyLoadAttempt = millis();
  apiUrl = buildMonthlyApiUrl(currentYear, currentMonth);

  Serial.println("Lade Monatsdaten...");
  Serial.print("selectedCity: ");
  Serial.println(selectedCity);
  Serial.print("API URL: ");
  Serial.println(apiUrl);

  bool ok = fetchMonthlyPrayerTimes(monthlyPrayerTimes, daysInLoadedMonth, apiUrl);

  if (ok) {
    loadedMonth = currentMonth;
    loadedYear = currentYear;

    Serial.print("Monatsdaten geladen. Monat/Jahr: ");
    Serial.print(loadedMonth);
    Serial.print("/");
    Serial.print(loadedYear);
    Serial.print(" | Tage: ");
    Serial.println(daysInLoadedMonth);

    printTodayCacheDebug();
  } else {
    Serial.println("Monatsdaten konnten NICHT geladen werden.");
  }

  return ok;
}

String getNextPrayerTimeFor(const String& prayerName) {
  int todayIndex = currentDay - 1;
  int nowMinutes = timeClient.getHours() * 60 + timeClient.getMinutes();

  String todayTime = getPrayerTimeByName(todayIndex, prayerName);
  String tomorrowTime = getPrayerTimeByName(todayIndex + 1, prayerName);

  if (todayTime == "") {
    return "--:--";
  }

  int todayPrayerMinutes = timeToMinutes(todayTime);

  if (todayPrayerMinutes < 0) {
    return "--:--";
  }

  if (nowMinutes < todayPrayerMinutes) {
    return todayTime;
  }

  if (tomorrowTime != "") {
    return tomorrowTime;
  }

  return todayTime;
}

void refreshDisplayedPrayerTimes() {
    static unsigned long lastWarn = 0;

    if (daysInLoadedMonth <= 0) {
        if (millis() - lastWarn > 5000) {
            Serial.println("refreshDisplayedPrayerTimes: Monatsdaten nicht verfügbar.");
            lastWarn = millis();
        }
        return;
    }

    fajrTime = getNextPrayerTimeFor("Fajr");
    shurukTime = getNextPrayerTimeFor("Shuruk");
    dhuhrTime = getNextPrayerTimeFor("Dhuhr");
    asrTime = getNextPrayerTimeFor("Asr");
    maghribTime = getNextPrayerTimeFor("Maghrib");
    ishaTime = getNextPrayerTimeFor("Isha");
}

String getTodayPrayerTimeFor(const String& prayerName) {
  int todayIndex = currentDay - 1;
  return getPrayerTimeByName(todayIndex, prayerName);
}

bool isTimeForReminder(String prayerTime, bool& reminderPlayed, int reminderMode) {
  if (reminderMode == 0) return false;
  if (prayerTime.length() < 5) return false;

  String currentTime = timeClient.getFormattedTime().substring(0, 5);

  int reminderOffset = (reminderMode == 1) ? 15 : 30;
  int prayerHour = prayerTime.substring(0, 2).toInt();
  int prayerMinute = prayerTime.substring(3, 5).toInt();

  int reminderHour = prayerHour;
  int reminderMinute = prayerMinute - reminderOffset;

  if (reminderMinute < 0) {
    reminderMinute += 60;
    reminderHour -= 1;
    if (reminderHour < 0) {
      reminderHour = 23;
    }
  }

  String reminderTime = (reminderHour < 10 ? "0" : "") + String(reminderHour) + ":" +
                        (reminderMinute < 10 ? "0" : "") + String(reminderMinute);

  if (currentTime == reminderTime && !reminderPlayed) {
    reminderPlayed = true;
    return true;
  }

  if (currentTime != reminderTime) {
    reminderPlayed = false;
  }

  return false;
}

bool shouldPlayAthan(String prayerTime, bool& athanPlayed, int athanMode) {
  if (athanMode == 0) return false;
  if (prayerTime.length() < 5) return false;

  String currentTime = timeClient.getFormattedTime().substring(0, 5);

  if (currentTime == prayerTime && !athanPlayed) {
    athanPlayed = true;
    return true;
  }

  if (currentTime != prayerTime) {
    athanPlayed = false;
  }

  return false;
}

void startPrayerCountdown(String prayerName, String prayerTime) {
  if (prayerTime.length() < 5) {
    countdownActive = false;
    return;
  }

  int prayerHour = prayerTime.substring(0, 2).toInt();
  int prayerMinute = prayerTime.substring(3, 5).toInt();

  unsigned long prayerSeconds = prayerHour * 3600UL + prayerMinute * 60UL;

  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();
  unsigned long nowSeconds = currentHour * 3600UL + currentMinute * 60UL + currentSecond;

  long remainingSeconds = (long)prayerSeconds - (long)nowSeconds;

  if (remainingSeconds <= 0) {
    countdownActive = false;
    return;
  }

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
  Serial.print(" | Restsekunden: ");
  Serial.println(remainingSeconds);
}

void updatePrayerCountdown() {
  if (!countdownActive) return;

  unsigned long now = millis();
  if (now - lastCountdownUpdate < 500) return;
  lastCountdownUpdate = now;

  long remainingMillis = (long)countdownEndMillis - (long)now;
  if (remainingMillis <= 0) {
    countdownActive = false;
    isReminderActive = false;
    Serial.print("Countdown fertig für ");
    Serial.println(countdownPrayerName);
    return;
  }

  int remainingSeconds = remainingMillis / 1000;
  if (remainingSeconds == lastCountdownSecondsShown) {
    return;
  }
  lastCountdownSecondsShown = remainingSeconds;

  showPrayerReminder(display, countdownPrayerName, (unsigned long)remainingSeconds);
}

void printDisplayTimesDebug() {
  Serial.print("Anzeige -> Fajr: "); Serial.print(fajrTime);
  Serial.print(" | Shuruk: "); Serial.print(shurukTime);
  Serial.print(" | Dhuhr: "); Serial.print(dhuhrTime);
  Serial.print(" | Asr: "); Serial.print(asrTime);
  Serial.print(" | Maghrib: "); Serial.print(maghribTime);
  Serial.print(" | Isha: "); Serial.println(ishaTime);
}

void setup() {
  Serial.begin(115200);

  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_WHITE);

  display.setFont(NULL);
  display.setTextColor(ST77XX_BLACK);
  display.fillScreen(ST77XX_WHITE);

  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds("Athan", 0, 0, &x1, &y1, &w, &h);
  int16_t xAthan = (display.width() - w) / 2;
  display.setCursor(xAthan, 20);
  display.print("Athan");

  display.getTextBounds("Clock", 0, 0, &x1, &y1, &w, &h);
  int16_t xClock = (display.width() - w) / 2;
  display.setCursor(xClock, 50);
  display.print("Clock");

  showBootMessage("DFPlayer init...");
  setupAudio();

  showBootMessage("WLAN verbinden...");
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Mit WLAN verbunden!");
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.localIP());
  showBootMessage("WLAN verbunden!");

  timeClient.begin();
  timeClient.update();
  updateCurrentDateFromNTP();
  printCurrentDateDebug();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/setCity", HTTP_POST, handleSetCity);
  server.on("/setAthan", HTTP_POST, handleSetAthan);
  server.begin();
  Serial.println("HTTP Server gestartet.");

  showBootMessage("Zeiten abrufen...");
  loadMonthlyPrayerTimesIfNeeded(true);
  refreshDisplayedPrayerTimes();
  printDisplayTimesDebug();
  showBootMessage("Zeiten Abruf erfolgreich!");
  delay(1000);

  setupButtons();
  showBootMessage("Buttons init...");

  delay(1000);
  display.fillScreen(ST77XX_WHITE);
  display.setCursor(10, 10);
  display.setTextSize(1);
  showBootMessage("Boot abgeschlossen!");
  Serial.println("Spiele Boot Ton ab...");
  playReminder(reminderTone);

  currentMode = MODE_HOME;
  Serial.println("Start: MODE_HOME");
}

void loop() {
  timeClient.update();
  updateCurrentDateFromNTP();

  server.handleClient();
  handleButtons();

  if (currentDay != lastDisplayDay) {
    Serial.println("Neuer Tag erkannt -> Monatsdaten prüfen.");
    loadMonthlyPrayerTimesIfNeeded(false);
    lastDisplayDay = currentDay;
  }

  if ((currentMonth != loadedMonth) || (currentYear != loadedYear)) {
    loadMonthlyPrayerTimesIfNeeded(false);
  }

  refreshDisplayedPrayerTimes();

  if (millis() - lastDebugPrint > 10000) {
    lastDebugPrint = millis();
    printCurrentDateDebug();
    printTodayCacheDebug();
    printDisplayTimesDebug();
  }

  if (currentMode == MODE_HOME) {
    updatePrayerCountdown();
  }

  if (currentMode == MODE_HOME && !isReminderActive && !countdownActive) {
    int currentMinute = timeClient.getMinutes();
    if (currentMinute != lastUpdatedMinute) {
      updateDisplay(display, fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, timeClient.getFormattedTime());
      lastUpdatedMinute = currentMinute;
    }
  }

  if (!isReminderActive && !countdownActive) {
    int currentMinute = timeClient.getMinutes();
    if (currentMinute != lastUpdatedMinute) {
      updateDisplay(display, fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, timeClient.getFormattedTime());
      lastUpdatedMinute = currentMinute;
    }
  }

  if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0 && millis() - lastPrayerUpdate > 60 * 1000) {
    Serial.println("Mitternacht erkannt -> Daten aktualisieren.");
    loadMonthlyPrayerTimesIfNeeded(true);
    refreshDisplayedPrayerTimes();
    lastPrayerUpdate = millis();
  }

  String todayFajr = getTodayPrayerTimeFor("Fajr");
  String todayShuruk = getTodayPrayerTimeFor("Shuruk");
  String todayDhuhr = getTodayPrayerTimeFor("Dhuhr");
  String todayAsr = getTodayPrayerTimeFor("Asr");
  String todayMaghrib = getTodayPrayerTimeFor("Maghrib");
  String todayIsha = getTodayPrayerTimeFor("Isha");

  if (isTimeForReminder(todayFajr, fajrReminderPlayed, prayerReminderModes[0])) {
    if (!countdownActive || countdownPrayerName != "Fajr") {
      startPrayerCountdown("Fajr", todayFajr);
    }
    isReminderActive = true;
    playReminder(reminderTone);
  } else if (isTimeForReminder(todayShuruk, shurukReminderPlayed, prayerReminderModes[1])) {
    if (!countdownActive || countdownPrayerName != "Shuruk") {
      startPrayerCountdown("Shuruk", todayShuruk);
    }
    isReminderActive = true;
    playReminder(reminderTone);
  } else if (isTimeForReminder(todayDhuhr, dhuhrReminderPlayed, prayerReminderModes[2])) {
    if (!countdownActive || countdownPrayerName != "Dhuhr") {
      startPrayerCountdown("Dhuhr", todayDhuhr);
    }
    isReminderActive = true;
    playReminder(reminderTone);
  } else if (isTimeForReminder(todayAsr, asrReminderPlayed, prayerReminderModes[3])) {
    if (!countdownActive || countdownPrayerName != "Asr") {
      startPrayerCountdown("Asr", todayAsr);
    }
    isReminderActive = true;
    playReminder(reminderTone);
  } else if (isTimeForReminder(todayMaghrib, maghribReminderPlayed, prayerReminderModes[4])) {
    if (!countdownActive || countdownPrayerName != "Maghrib") {
      startPrayerCountdown("Maghrib", todayMaghrib);
    }
    isReminderActive = true;
    playReminder(reminderTone);
  } else if (isTimeForReminder(todayIsha, ishaReminderPlayed, prayerReminderModes[5])) {
    if (!countdownActive || countdownPrayerName != "Isha") {
      startPrayerCountdown("Isha", todayIsha);
    }
    isReminderActive = true;
    playReminder(reminderTone);
  } else {
    isReminderActive = false;
    countdownActive = false;
  }

  updatePrayerCountdown();

  if (shouldPlayAthan(todayFajr, fajrAthanPlayed, prayerAthanModes[0]) ||
      shouldPlayAthan(todayShuruk, shurukAthanPlayed, prayerAthanModes[1]) ||
      shouldPlayAthan(todayDhuhr, dhuhrAthanPlayed, prayerAthanModes[2]) ||
      shouldPlayAthan(todayAsr, asrAthanPlayed, prayerAthanModes[3]) ||
      shouldPlayAthan(todayMaghrib, maghribAthanPlayed, prayerAthanModes[4]) ||
      shouldPlayAthan(todayIsha, ishaAthanPlayed, prayerAthanModes[5])) {
    Serial.println("Athan wird abgespielt.");
    playAthan(athanTone);
  }

  delay(50);
}