#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP8266WiFi.h>
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

#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define MY_NTP_SERVER "de.pool.ntp.org"
#define DISPLAY_REFRESH_MS 250
#define MAIN_LOOP_DELAY_MS 50
#define PRAYER_COUNT 6

enum PrayerIndex {
  PRAYER_FAJR = 0,
  PRAYER_SHURUK,
  PRAYER_DHUHR,
  PRAYER_ASR,
  PRAYER_MAGHRIB,
  PRAYER_ISHA
};

const char* prayerNames[PRAYER_COUNT] = {
  "Fajr", "Shuruk", "Dhuhr", "Asr", "Maghrib", "Isha"
};

Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);

// Diese globalen Variablen bleiben erhalten, damit bestehende .h/.cpp-Dateien
// wie display.h, api.h oder html.h weiterhin korrekt linken können.
String fajrTime = "";
String shurukTime = "";
String dhuhrTime = "";
String asrTime = "";
String maghribTime = "";
String ishaTime = "";

String prayerTimes[PRAYER_COUNT];
bool reminderPlayed[PRAYER_COUNT] = {false, false, false, false, false, false};
int prayerReminderModes[PRAYER_COUNT] = {1, 1, 1, 1, 1, 1};
int prayerAthanModes[PRAYER_COUNT] = {1, 1, 1, 1, 1, 1};

int currentDay, currentMonth, currentYear;
String apiUrl;
String MAWAQIT_URL;
String selectedCity = "Mainz";
String reminderTone = "0";
String athanTone = "0";

unsigned long lastPrayerUpdate = 0;
int lastUpdatedMinute = -1;
unsigned long lastDisplayRefresh = 0;
int lastAthanMinuteOfDay = -1;
bool timeValid = false;

bool isReminderActive = false;
bool countdownActive = false;
String countdownPrayerName = "";
unsigned long countdownEndMillis = 0;
unsigned long lastCountdownUpdate = 0;
int lastCountdownSecondsShown = -1;

IPAddress staticIP(192, 168, 1, 255);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

String getFormattedTime();
bool getLocalTimeStruct(struct tm &timeinfo);
int getCurrentMinuteOfDay();
void updateCurrentDate();
bool syncClock(unsigned long timeoutMs = 15000);
void updatePrayerVariablesFromArray();
void updateArrayFromPrayerVariables();
void refreshHomeDisplayIfNeeded(bool force = false);
void updatePrayerCountdown();
void processReminderAndCountdown();
void processAthanPlayback();
void fetchPrayerTimesSafe();
bool isTimeForReminder(const String &prayerTime, bool &alreadyPlayed, int reminderMode);
bool shouldPlayAthanNow(const String &prayerTime, int athanMode);
void startPrayerCountdown(const String &prayerName, const String &prayerTime);

void showBootMessage(const char* message) {
  display.fillRect(0, 70, 128, 50, ST77XX_WHITE);
  display.setCursor(10, 80);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.print(message);
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTimeStruct(timeinfo)) {
    return "--:--:--";
  }

  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buf);
}

bool getLocalTimeStruct(struct tm &timeinfo) {
  time_t now = time(nullptr);
  if (now < 100000) {
    return false;
  }

  struct tm *ptm = localtime(&now);
  if (ptm == nullptr) {
    return false;
  }

  timeinfo = *ptm;
  return true;
}

int getCurrentMinuteOfDay() {
  struct tm timeinfo;
  if (!getLocalTimeStruct(timeinfo)) {
    return -1;
  }
  return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

void updateCurrentDate() {
  struct tm timeinfo;
  if (!getLocalTimeStruct(timeinfo)) {
    return;
  }

  currentDay = timeinfo.tm_mday;
  currentMonth = timeinfo.tm_mon + 1;
  currentYear = timeinfo.tm_year + 1900;
}

bool syncClock(unsigned long timeoutMs) {
  configTime(MY_TZ, MY_NTP_SERVER);

  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (time(nullptr) > 100000) {
      timeValid = true;
      return true;
    }
    delay(250);
  }

  timeValid = false;
  return false;
}

void updatePrayerVariablesFromArray() {
  fajrTime = prayerTimes[PRAYER_FAJR];
  shurukTime = prayerTimes[PRAYER_SHURUK];
  dhuhrTime = prayerTimes[PRAYER_DHUHR];
  asrTime = prayerTimes[PRAYER_ASR];
  maghribTime = prayerTimes[PRAYER_MAGHRIB];
  ishaTime = prayerTimes[PRAYER_ISHA];
}

void updateArrayFromPrayerVariables() {
  prayerTimes[PRAYER_FAJR] = fajrTime;
  prayerTimes[PRAYER_SHURUK] = shurukTime;
  prayerTimes[PRAYER_DHUHR] = dhuhrTime;
  prayerTimes[PRAYER_ASR] = asrTime;
  prayerTimes[PRAYER_MAGHRIB] = maghribTime;
  prayerTimes[PRAYER_ISHA] = ishaTime;
}

void fetchPrayerTimesSafe() {
  fetchPrayerTimes(fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, MAWAQIT_URL);
  updateArrayFromPrayerVariables();
}

void refreshHomeDisplayIfNeeded(bool force) {
  if (currentMode != MODE_HOME || isReminderActive || countdownActive) {
    return;
  }

  unsigned long nowMs = millis();
  if (!force && nowMs - lastDisplayRefresh < DISPLAY_REFRESH_MS) {
    return;
  }

  struct tm timeinfo;
  if (!getLocalTimeStruct(timeinfo)) {
    return;
  }

  if (force || timeinfo.tm_min != lastUpdatedMinute) {
    updateDisplay(display,
                  fajrTime,
                  shurukTime,
                  dhuhrTime,
                  asrTime,
                  maghribTime,
                  ishaTime,
                  getFormattedTime());
    lastUpdatedMinute = timeinfo.tm_min;
    lastDisplayRefresh = nowMs;
  }
}

bool isTimeForReminder(const String &prayerTime, bool &alreadyPlayed, int reminderMode) {
  if (reminderMode == 0 || prayerTime.length() < 5) {
    return false;
  }

  String currentTime = getFormattedTime().substring(0, 5);
  if (currentTime == "--:--") {
    return false;
  }

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

  if (currentTime == reminderTime && !alreadyPlayed) {
    alreadyPlayed = true;
    return true;
  }

  if (currentTime != reminderTime) {
    alreadyPlayed = false;
  }

  return false;
}

bool shouldPlayAthanNow(const String &prayerTime, int athanMode) {
  if (athanMode == 0 || prayerTime.length() < 5) {
    return false;
  }

  String currentTime = getFormattedTime().substring(0, 5);
  if (currentTime == "--:--") {
    return false;
  }

  return currentTime == prayerTime;
}

void startPrayerCountdown(const String &prayerName, const String &prayerTime) {
  if (prayerTime.length() < 5) {
    countdownActive = false;
    return;
  }

  int prayerHour = prayerTime.substring(0, 2).toInt();
  int prayerMinute = prayerTime.substring(3, 5).toInt();
  unsigned long prayerSeconds = prayerHour * 3600UL + prayerMinute * 60UL;

  struct tm timeinfo;
  if (!getLocalTimeStruct(timeinfo)) {
    countdownActive = false;
    return;
  }

  unsigned long nowSeconds = timeinfo.tm_hour * 3600UL + timeinfo.tm_min * 60UL + timeinfo.tm_sec;
  long remainingSeconds = (long)prayerSeconds - (long)nowSeconds;

  if (remainingSeconds <= 0 || remainingSeconds > 15 * 60) {
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

  unsigned long nowMs = millis();
  if (nowMs - lastCountdownUpdate < 500) return;
  lastCountdownUpdate = nowMs;

  long remainingMillis = (long)countdownEndMillis - (long)nowMs;
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

void processReminderAndCountdown() {
  bool triggered = false;

  for (int i = 0; i < PRAYER_COUNT; i++) {
    if (isTimeForReminder(prayerTimes[i], reminderPlayed[i], prayerReminderModes[i])) {
      if (!countdownActive || countdownPrayerName != prayerNames[i]) {
        startPrayerCountdown(prayerNames[i], prayerTimes[i]);
      }
      isReminderActive = true;
      playReminder(reminderTone);
      triggered = true;
      break;
    }
  }

  if (!triggered && !countdownActive) {
    isReminderActive = false;
  }
}

void processAthanPlayback() {
  int minuteOfDay = getCurrentMinuteOfDay();
  if (minuteOfDay < 0) {
    return;
  }

  if (minuteOfDay == lastAthanMinuteOfDay) {
    return;
  }

  for (int i = 0; i < PRAYER_COUNT; i++) {
    if (shouldPlayAthanNow(prayerTimes[i], prayerAthanModes[i])) {
      playAthan(athanTone);
      lastAthanMinuteOfDay = minuteOfDay;
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);

  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_WHITE);
  display.setFont(NULL);
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(2);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds("Athan", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, 20);
  display.print("Athan");

  display.getTextBounds("Clock", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, 50);
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
  Serial.println("\nMit WLAN verbunden!");
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.localIP());

  showBootMessage("Zeit synchronisieren...");
  if (syncClock()) {
    updateCurrentDate();
    Serial.print("Lokale Zeit: ");
    Serial.println(getFormattedTime());
  } else {
    Serial.println("Zeit konnte nicht synchronisiert werden.");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/setCity", HTTP_POST, handleSetCity);
  server.on("/setAthan", HTTP_POST, handleSetAthan);
  server.begin();

  MAWAQIT_URL = "https://mawaqit.net/de/ikv-kostheim";

  showBootMessage("Zeiten abrufen...");
  fetchPrayerTimesSafe();
  showBootMessage("Zeiten geladen");
  delay(800);

  setupButtons();

  display.fillScreen(ST77XX_WHITE);
  showBootMessage("Boot abgeschlossen!");
  playReminder(reminderTone);

  currentMode = MODE_HOME;
  refreshHomeDisplayIfNeeded(true);
  Serial.println("Start: MODE_HOME");
}

void loop() {
  server.handleClient();
  handleButtons();
  updateVolumeOverlay(); 

  if (!timeValid && WiFi.status() == WL_CONNECTED) {
    timeValid = syncClock(2000);
  }

  if (timeValid) {
    updateCurrentDate();
  }

  int minuteOfDay = getCurrentMinuteOfDay();
  if (minuteOfDay == 0 && millis() - lastPrayerUpdate > 60UL * 1000UL) {
    fetchPrayerTimesSafe();
    lastPrayerUpdate = millis();
    Serial.println("Gebetszeiten um Mitternacht aktualisiert.");
  }

  processReminderAndCountdown();
  updatePrayerCountdown();
  processAthanPlayback();
  refreshHomeDisplayIfNeeded();

  delay(MAIN_LOOP_DELAY_MS);
}
