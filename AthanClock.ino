#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP8266WiFi.h>
#include <time.h>

#include "api.h"
#include "audio.h"
#include "buttons.h"
#include "config.h"
#include "display.h"
#include "html.h"
#include "menu.h"
#include "prayer_logic.h"
#include "settings.h"
#include "wifi_setup.h"

// Display-Pins
#define TFT_CS D8
#define TFT_RST D4
#define TFT_DC D3

// ---- Einstellungen ----
// Mitteleuropäische Zeit inkl. automatischer Sommerzeit (POSIX-TZ-Format)
static const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";
static const char* NTP_SERVER = "pool.ntp.org";
static const char* COUNTRY = "Germany";  // nur für alte Einstellungen ohne Koordinaten
static const unsigned long FETCH_RETRY_MS = 60UL * 1000UL;  // aktueller Monat fehlt
static const unsigned long PRELOAD_RETRY_MS = 5UL * 60UL * 1000UL;
static const int PRELOAD_DAYS_BEFORE_END = 3;  // Folgemonat ab 3 Tage vor Monatsende laden
static const unsigned long WIFI_TIMEOUT_MS = 20000UL;
static const unsigned long NTP_TIMEOUT_MS = 15000UL;

Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);

// Einstellungen (werden von settings.cpp geladen/gespeichert)
Location currentLocation = {"Mainz", "Rheinland-Pfalz", 49.98185, 8.28008, true};
int prayerReminderModes[6] = {1, 1, 1, 1, 1, 1};
int prayerAthanModes[6] = {1, 0, 1, 1, 1, 1};  // Shuruk: kein Athan
String reminderTone = "0";
String athanTone = "0";
String fajrTone = "0";
int fajrVolume = 12;  // nachts leiser
int calcMethod = 2;  // aladhan-Berechnungsmethode (2 = ISNA)
int asrSchool = 0;   // 0 = Standard, 1 = Hanafi
int prayerOffsets[6] = {0, 0, 0, 0, 0, 0};

// Monatsdaten
static prayer::MonthTimes currentMonthTimes;
static prayer::MonthTimes nextMonthTimes;
static prayer::MonthTimes scratchMonthTimes;  // Zwischenspeicher für Abrufe

static prayer::Scheduler scheduler;
static int shownTimes[prayer::kPrayerCount] = {-1, -1, -1, -1, -1, -1};  // nächstes Vorkommen
static int todayTimes[prayer::kPrayerCount] = {-1, -1, -1, -1, -1, -1};  // heutige Zeiten

static bool currentAttempted = false;
static unsigned long lastCurrentAttempt = 0;
static bool nextAttempted = false;
static unsigned long lastNextAttempt = 0;

// Anzeige-Zustand
static time_t lastTick = 0;
static prayer::Events lastEvents = {-1, -1, -1, 0};
static int lastDrawnMinute = -1;
static int shownReminder = -1;
static long shownCountdown = -1;
static bool waitingScreenShown = false;

void showBootMessage(const char* message) {
  display.fillRect(0, 70, 128, 50, ST77XX_WHITE);
  display.setCursor(10, 80);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.print(message);
}

bool readLocalTime(struct tm& t) {
  time_t now = time(nullptr);
  if (now < 1600000000) return false;  // vor 2020 -> noch keine gültige Zeit
  localtime_r(&now, &t);
  return true;
}

void requestScreenRedraw() {
  lastDrawnMinute = -1;
  shownReminder = -1;
  shownCountdown = -1;
  waitingScreenShown = false;
}

void getPrayerOverview(int today[6], int upcoming[6]) {
  memcpy(today, todayTimes, sizeof(todayTimes));
  memcpy(upcoming, shownTimes, sizeof(shownTimes));
}

static bool loadMonth(prayer::MonthTimes& out, const Location& location, int method, int school, int year,
                      int month) {
  char url[256];
  bool ok = location.hasCoords
                ? prayer::buildCalendarUrlByCoords(url, sizeof(url), year, month, location.latitude,
                                                   location.longitude, method, school)
                : prayer::buildCalendarUrl(url, sizeof(url), year, month, location.name.c_str(), COUNTRY, method,
                                           school);
  if (!ok) {
    Serial.println("API-URL konnte nicht gebaut werden.");
    return false;
  }
  return fetchMonthlyPrayerTimes(url, year, month, out);
}

static bool fail(String* error, const char* message) {
  if (error) *error = message;
  Serial.print("changeLocation: ");
  Serial.println(message);
  return false;
}

bool changeLocation(const Location& requested, int method, int school, String* error) {
  Location location = requested;
  location.name.trim();
  location.region.trim();
  if (location.name.length() == 0 || location.name.length() > 60 || location.region.length() > 60) {
    return fail(error, "Ungültiger Ortsname.");
  }
  if (location.hasCoords && !prayer::isValidCoordinate(location.latitude, location.longitude)) {
    return fail(error, "Ungültige Koordinaten.");
  }
  if (!prayer::isValidMethod(method)) return fail(error, "Ungültige Berechnungsmethode.");
  if (school != 0 && school != 1) return fail(error, "Ungültige Asr-Berechnung.");

  struct tm t;
  if (!readLocalTime(t)) return fail(error, "Uhrzeit noch nicht synchronisiert.");

  // Erst laden, dann prüfen, dann übernehmen: bei Fehler bleibt alles wie es war.
  if (!loadMonth(scratchMonthTimes, location, method, school, t.tm_year + 1900, t.tm_mon + 1)) {
    return fail(error, "Zeiten konnten nicht geladen werden. Bitte später erneut versuchen.");
  }

  // Die Uhr läuft in MEZ/MESZ. Orte in anderen Zeitzonen würden falsche Zeiten anzeigen.
  if (scratchMonthTimes.timezone[0] != '\0' && !prayer::isCentralEuropeanTimezone(scratchMonthTimes.timezone)) {
    static char message[128];
    snprintf(message, sizeof(message), "Ort liegt in der Zeitzone %s. Die Uhr unterstützt nur MEZ/MESZ.",
             scratchMonthTimes.timezone);
    return fail(error, message);
  }

  currentMonthTimes = scratchMonthTimes;
  prayer::clearMonth(nextMonthTimes);  // Folgemonat gehörte zum alten Ort/zur alten Methode
  nextAttempted = false;
  currentAttempted = false;
  currentLocation = location;
  calcMethod = method;
  asrSchool = school;
  markSettingsDirty();
  requestScreenRedraw();
  Serial.printf("Ort gewechselt: %s, Methode %d, Zeitzone %s\n", currentLocation.name.c_str(), calcMethod,
                currentMonthTimes.timezone);
  return true;
}

// Sorgt dafür, dass der aktuelle Monat geladen ist und der Folgemonat
// rechtzeitig vor Monatsende vorliegt.
static void manageMonthData(const struct tm& t) {
  int year = t.tm_year + 1900;
  int month = t.tm_mon + 1;
  int day = t.tm_mday;
  unsigned long nowMs = millis();

  if (prayer::promoteNextMonth(currentMonthTimes, nextMonthTimes, year, month)) {
    Serial.println("Monatswechsel: vorgeladenen Folgemonat übernommen.");
    nextAttempted = false;
    requestScreenRedraw();
  }

  if (!prayer::monthMatches(currentMonthTimes, year, month)) {
    if (currentAttempted && nowMs - lastCurrentAttempt < FETCH_RETRY_MS) return;
    currentAttempted = true;
    lastCurrentAttempt = nowMs;
    Serial.println("Lade aktuellen Monat...");
    if (loadMonth(scratchMonthTimes, currentLocation, calcMethod, asrSchool, year, month)) {
      currentMonthTimes = scratchMonthTimes;
      currentAttempted = false;
      requestScreenRedraw();
    } else {
      Serial.println("Aktueller Monat fehlgeschlagen, neuer Versuch in 60 s.");
    }
    return;
  }

  if (day > prayer::daysInMonth(year, month) - PRELOAD_DAYS_BEFORE_END) {
    int nextYear, nextMonth;
    prayer::addMonths(year, month, 1, nextYear, nextMonth);
    if (prayer::monthMatches(nextMonthTimes, nextYear, nextMonth)) return;
    if (nextAttempted && nowMs - lastNextAttempt < PRELOAD_RETRY_MS) return;
    nextAttempted = true;
    lastNextAttempt = nowMs;
    Serial.println("Lade Folgemonat vor...");
    if (loadMonth(scratchMonthTimes, currentLocation, calcMethod, asrSchool, nextYear, nextMonth)) {
      nextMonthTimes = scratchMonthTimes;
      nextAttempted = false;
    }
  }
}

static void processTick(const struct tm& t) {
  int year = t.tm_year + 1900;
  int month = t.tm_mon + 1;
  int day = t.tm_mday;

  manageMonthData(t);

  int ty, tm_, td;
  prayer::nextDay(year, month, day, ty, tm_, td);
  const int16_t* rawToday = prayer::timesForDate(currentMonthTimes, nextMonthTimes, year, month, day);
  const int16_t* rawTomorrow = prayer::timesForDate(currentMonthTimes, nextMonthTimes, ty, tm_, td);

  // Minuten-Korrektur pro Gebet einrechnen (gilt für Anzeige, Erinnerung und Athan)
  int16_t todayAdjusted[prayer::kPrayerCount], tomorrowAdjusted[prayer::kPrayerCount];
  prayer::applyOffsets(rawToday, prayerOffsets, todayAdjusted);
  prayer::applyOffsets(rawTomorrow, prayerOffsets, tomorrowAdjusted);
  const int16_t* today = rawToday ? todayAdjusted : nullptr;
  const int16_t* tomorrow = rawTomorrow ? tomorrowAdjusted : nullptr;

  int nowMinute = t.tm_hour * 60 + t.tm_min;
  int newShown[prayer::kPrayerCount];
  prayer::displayTimes(today, tomorrow, nowMinute, newShown);
  for (int i = 0; i < prayer::kPrayerCount; i++) todayTimes[i] = today ? today[i] : -1;
  if (memcmp(newShown, shownTimes, sizeof(shownTimes)) != 0) {
    memcpy(shownTimes, newShown, sizeof(shownTimes));
    lastDrawnMinute = -1;
  }

  scheduler.setTimes(today);
  scheduler.setModes(prayerAthanModes, prayerReminderModes);
  long dayKey = prayer::daysFromCivil(year, month, day);
  long nowSec = t.tm_hour * 3600L + t.tm_min * 60L + t.tm_sec;
  lastEvents = scheduler.update(dayKey, nowSec);

  if (lastEvents.reminderStarted >= 0) {
    Serial.printf("Erinnerung: %s\n", prayer::kNames[lastEvents.reminderStarted]);
    // Vor Fajr ist es Nacht: Erinnerung mit der leiseren Fajr-Lautstärke
    playReminder(reminderTone, lastEvents.reminderStarted == prayer::FAJR ? fajrVolume : -1);
  }
  if (lastEvents.athanDue >= 0) {
    Serial.printf("Athan: %s\n", prayer::kNames[lastEvents.athanDue]);
    if (lastEvents.athanDue == prayer::FAJR) {
      playFajrAthan(fajrTone, fajrVolume);
    } else {
      playAthan(athanTone);
    }
  }
}

// Zeichnet nur, was sich geändert hat. Läuft nur auf dem Startbildschirm,
// damit Menü und Bearbeitungsansicht nie übermalt werden.
static void render(const struct tm& t) {
  if (currentMode != MODE_HOME) return;

  if (lastEvents.activeReminder >= 0) {
    if (shownReminder != lastEvents.activeReminder) {
      showReminderScreen(display, prayer::kNames[lastEvents.activeReminder]);
      shownReminder = lastEvents.activeReminder;
      shownCountdown = -1;
    }
    if (shownCountdown != lastEvents.reminderSecondsLeft) {
      updateReminderCountdown(display, lastEvents.reminderSecondsLeft);
      shownCountdown = lastEvents.reminderSecondsLeft;
    }
    return;
  }

  if (shownReminder >= 0) {
    shownReminder = -1;
    lastDrawnMinute = -1;
  }
  if (t.tm_min != lastDrawnMinute) {
    char cityAscii[64];
    prayer::transliterateForDisplay(currentLocation.name.c_str(), cityAscii, sizeof(cityAscii));
    updateDisplay(display, cityAscii, shownTimes, t.tm_hour, t.tm_min);
    lastDrawnMinute = t.tm_min;
  }
}

void setup() {
  Serial.begin(115200);

  setupDisplay(display);
  display.setFont(NULL);
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("Athan", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, 20);
  display.print("Athan");
  display.getTextBounds("Clock", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, 50);
  display.print("Clock");

  showBootMessage("Einstellungen...");
  loadSettings();

  showBootMessage("DFPlayer init...");
  setupAudio();

  showBootMessage("Buttons init...");
  setupButtons();

  // WLAN: gespeicherte Daten nutzen; sonst (oder Taste UP gedrückt) Hotspot zur Einrichtung
  initDeviceIdentity();
  loadWifiCredentials();
  bool forceSetup = isUpButtonPressed();
  bool connected = false;
  if (forceSetup) {
    Serial.println("Taste UP beim Start gedrueckt -> WLAN-Einrichtung.");
  } else if (hasWifiCredentials()) {
    showBootMessage("WLAN verbinden...");
    connected = connectWifi(WIFI_TIMEOUT_MS);
  }
  if (connected) {
    showBootMessage("WLAN verbunden!");
  } else {
    startSetupPortal();
  }

  // Zeit per NTP, Zeitzone inkl. Sommerzeit (läuft auch los, wenn WLAN erst später kommt)
  configTime(TZ_INFO, NTP_SERVER);
  struct tm t;
  if (connected) {
    showBootMessage("Zeit synchronisieren...");
    unsigned long ntpStart = millis();
    while (!readLocalTime(t) && millis() - ntpStart < NTP_TIMEOUT_MS) delay(200);
  }

  setupWebServer();

  prayer::clearMonth(currentMonthTimes);
  prayer::clearMonth(nextMonthTimes);
  if (readLocalTime(t)) {
    showBootMessage("Zeiten abrufen...");
    manageMonthData(t);
  } else if (connected) {
    showBootMessage("NTP Fehler");
    delay(1000);
  }

  currentMode = MODE_HOME;
  requestScreenRedraw();

  Serial.println("Spiele Boot Ton ab...");
  playReminder(reminderTone);
}

void loop() {
  updateWifi();
  updateWebServer();
  handleButtons();
  updateAudio();
  updateVolumeOverlay();
  updateMenuTimeout();
  saveSettingsIfDirty();

  struct tm t;
  if (!readLocalTime(t)) {
    // Ohne Uhrzeit: Einrichtungsanleitung (Hotspot) bzw. Warte-Hinweis.
    // Ist die Zeit bekannt, läuft die Uhr normal weiter, auch wenn der Hotspot offen ist.
    if (currentMode == MODE_HOME && !waitingScreenShown) {
      if (isSetupMode()) {
        showWifiSetupScreen(display, setupApName(), devicePassword(), hasWifiCredentials());
      } else {
        display.fillScreen(ST77XX_WHITE);
        showBootMessage("Warte auf Uhrzeit...");
      }
      waitingScreenShown = true;
    }
    delay(isSetupMode() ? 5 : 20);  // im Hotspot DNS zügig beantworten
    return;
  }

  time_t now = time(nullptr);
  if (now != lastTick) {
    lastTick = now;
    processTick(t);
  }
  render(t);

  delay(20);
}
