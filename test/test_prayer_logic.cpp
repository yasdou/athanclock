// Unit-Tests für prayer_logic.cpp. Laufen auf dem PC, ohne Mikrocontroller.
//
//   (im Ordner src/)
//   g++ -std=c++11 -Wall -Wextra -I. test/test_prayer_logic.cpp prayer_logic.cpp -o test_prayer_logic
//   ./test_prayer_logic
//
// PlatformIO baut diesen Ordner nicht in die Firmware (build_src_filter in platformio.ini),
// die Arduino-IDE ignoriert ihn ebenfalls.

#include <stdio.h>
#include <string.h>

#include "prayer_logic.h"

using namespace prayer;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    checks++;                                                    \
    if (!(cond)) {                                               \
      failures++;                                                \
      printf("  FEHLER %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                            \
  } while (0)

#define CHECK_EQ(a, b)                                                                     \
  do {                                                                                     \
    checks++;                                                                              \
    long _a = (long)(a), _b = (long)(b);                                                   \
    if (_a != _b) {                                                                        \
      failures++;                                                                          \
      printf("  FEHLER %s:%d: %s == %ld, erwartet %ld\n", __FILE__, __LINE__, #a, _a, _b); \
    }                                                                                      \
  } while (0)

#define CHECK_STR(a, b)                                                                  \
  do {                                                                                   \
    checks++;                                                                            \
    if (strcmp((a), (b)) != 0) {                                                         \
      failures++;                                                                        \
      printf("  FEHLER %s:%d: \"%s\", erwartet \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
    }                                                                                    \
  } while (0)

static long hms(int h, int m, int s) { return h * 3600L + m * 60L + s; }

static const int kAllOn[kPrayerCount] = {1, 1, 1, 1, 1, 1};
static const int16_t kDay[kPrayerCount] = {330, 480, 780, 960, 1020, 1140};  // 05:30 08:00 13:00 16:00 17:00 19:00

static void fillMonth(MonthTimes& m, int year, int month, int base) {
  clearMonth(m);
  m.year = year;
  m.month = month;
  m.days = daysInMonth(year, month);
  for (int d = 0; d < m.days; d++)
    for (int i = 0; i < kPrayerCount; i++) m.t[d][i] = (int16_t)(kDay[i] + base + d);
}

// ---------------------------------------------------------------------------

static void testParseAndFormat() {
  CHECK_EQ(parseHHMM("05:30"), 330);
  CHECK_EQ(parseHHMM("23:59"), 1439);
  CHECK_EQ(parseHHMM("17:42 (CET)"), 17 * 60 + 42);  // aladhan-Suffix
  CHECK_EQ(parseHHMM(""), -1);
  CHECK_EQ(parseHHMM(NULL), -1);
  CHECK_EQ(parseHHMM("null"), -1);
  CHECK_EQ(parseHHMM("5:30"), -1);
  CHECK_EQ(parseHHMM("24:00"), -1);
  CHECK_EQ(parseHHMM("12:60"), -1);

  char buf[6];
  formatHHMM(330, buf, sizeof(buf));
  CHECK_STR(buf, "05:30");
  formatHHMM(-1, buf, sizeof(buf));
  CHECK_STR(buf, "--:--");
  formatCountdown(30 * 60, buf, sizeof(buf));
  CHECK_STR(buf, "30:00");
  formatCountdown(61, buf, sizeof(buf));
  CHECK_STR(buf, "01:01");
  formatCountdown(-5, buf, sizeof(buf));
  CHECK_STR(buf, "00:00");
}

static void testCalendar() {
  CHECK_EQ(daysFromCivil(1970, 1, 1), 0);
  CHECK_EQ(daysFromCivil(2027, 1, 1) - daysFromCivil(2026, 12, 31), 1);
  CHECK_EQ(daysInMonth(2026, 2), 28);
  CHECK_EQ(daysInMonth(2028, 2), 29);
  CHECK_EQ(daysInMonth(2100, 2), 28);
  CHECK_EQ(daysInMonth(2000, 2), 29);
  CHECK_EQ(daysInMonth(2026, 11), 30);
  CHECK_EQ(daysInMonth(2026, 12), 31);

  int y, m, d;
  addMonths(2026, 12, 1, y, m);
  CHECK(y == 2027 && m == 1);
  addMonths(2026, 1, -1, y, m);
  CHECK(y == 2025 && m == 12);
  nextDay(2026, 11, 30, y, m, d);
  CHECK(y == 2026 && m == 12 && d == 1);
  nextDay(2026, 12, 31, y, m, d);
  CHECK(y == 2027 && m == 1 && d == 1);
  nextDay(2028, 2, 28, y, m, d);
  CHECK(y == 2028 && m == 2 && d == 29);
}

// Regression: Leerzeichen/Sonderzeichen im Stadtnamen machten die URL kaputt.
static void testCalendarUrl() {
  char url[256];
  CHECK(buildCalendarUrl(url, sizeof(url), 2026, 11, "Mainz", "Germany", 2, 0));
  CHECK_STR(url, "https://api.aladhan.com/v1/calendarByCity/2026/11?city=Mainz&country=Germany&method=2&school=0");

  CHECK(buildCalendarUrl(url, sizeof(url), 2027, 1, "Bad Kreuznach", "Germany", 2, 0));
  CHECK_STR(url, "https://api.aladhan.com/v1/calendarByCity/2027/1?city=Bad%20Kreuznach&country=Germany&method=2&school=0");

  CHECK(buildCalendarUrl(url, sizeof(url), 2027, 1, "A&B=C", "Germany", 2, 0));  // kein Parameter-Einschleusen
  CHECK(strstr(url, "city=A%26B%3DC&") != NULL);

  CHECK(buildCalendarUrl(url, sizeof(url), 2027, 1, "K\xC3\xB6ln", "Germany", 2, 0));  // "Köln" in UTF-8
  CHECK(strstr(url, "city=K%C3%B6ln&") != NULL);

  char tiny[40];
  CHECK(!buildCalendarUrl(tiny, sizeof(tiny), 2027, 1, "Mainz", "Germany", 2, 0));
}

static void testCoordinateUrl() {
  char url[256];
  CHECK(buildCalendarUrlByCoords(url, sizeof(url), 2026, 11, 49.98185, 8.28008, 2, 0));
  CHECK_STR(url, "https://api.aladhan.com/v1/calendar/2026/11?latitude=49.98185&longitude=8.28008&method=2&school=0");

  // Negative Werte, Rundung und führende Nullen der Nachkommastellen
  CHECK(buildCalendarUrlByCoords(url, sizeof(url), 2027, 1, -33.9, -0.000004, 3, 1));
  CHECK_STR(url, "https://api.aladhan.com/v1/calendar/2027/1?latitude=-33.90000&longitude=0.00000&method=3&school=1");
  CHECK(buildCalendarUrlByCoords(url, sizeof(url), 2027, 1, 50.000019, 6.95, 13, 0));
  CHECK(strstr(url, "latitude=50.00002&longitude=6.95000&method=13&school=0") != NULL);

  CHECK(!buildCalendarUrlByCoords(url, sizeof(url), 2027, 1, 91.0, 8.0, 2, 0));   // ungültig
  CHECK(!buildCalendarUrlByCoords(url, sizeof(url), 2027, 1, 50.0, 181.0, 2, 0));
  char tiny[40];
  CHECK(!buildCalendarUrlByCoords(tiny, sizeof(tiny), 2027, 1, 50.0, 8.0, 2, 0));
}

static void testLocationChecks() {
  CHECK(isValidCoordinate(49.98, 8.28));
  CHECK(isValidCoordinate(-90, 180));
  CHECK(!isValidCoordinate(90.1, 0));
  CHECK(!isValidCoordinate(0, -180.5));
  double zero = 0.0;
  CHECK(!isValidCoordinate(zero / zero, 8.0));  // NaN

  CHECK(isCentralEuropeanTimezone("Europe/Berlin"));
  CHECK(isCentralEuropeanTimezone("Europe/Vienna"));
  CHECK(isCentralEuropeanTimezone("Europe/Paris"));
  CHECK(!isCentralEuropeanTimezone("Europe/Istanbul"));
  CHECK(!isCentralEuropeanTimezone("Europe/London"));
  CHECK(!isCentralEuropeanTimezone("Europe/Helsinki"));
  CHECK(!isCentralEuropeanTimezone("Africa/Casablanca"));
  CHECK(!isCentralEuropeanTimezone(""));
  CHECK(!isCentralEuropeanTimezone(NULL));
}

static void testTransliteration() {
  char out[32];
  transliterateForDisplay("M\xC3\xBCnchen", out, sizeof(out));
  CHECK_STR(out, "Muenchen");
  transliterateForDisplay("K\xC3\xB6ln", out, sizeof(out));
  CHECK_STR(out, "Koeln");
  transliterateForDisplay("\xC3\x9C" "berlingen", out, sizeof(out));
  CHECK_STR(out, "Ueberlingen");
  transliterateForDisplay("Gie\xC3\x9F" "en", out, sizeof(out));
  CHECK_STR(out, "Giessen");
  transliterateForDisplay("Mainz", out, sizeof(out));
  CHECK_STR(out, "Mainz");
  transliterateForDisplay("\xC5\x81\xC3\xB3" "d\xC5\xBA", out, sizeof(out));  // Łódź
  CHECK_STR(out, "?od?");
  transliterateForDisplay("M\xC3\xBCnchen", out, 4);  // Puffer zu klein -> sauber abgeschnitten
  CHECK_STR(out, "Mue");
  transliterateForDisplay(NULL, out, sizeof(out));
  CHECK_STR(out, "");
}

static void testModes() {
  CHECK(!hasAthan(SHURUK));
  CHECK(hasAthan(FAJR) && hasAthan(ISHA));
  CHECK(!hasAthan(-1) && !hasAthan(kPrayerCount));
  CHECK_EQ(reminderLeadMinutes(0), 0);
  CHECK_EQ(reminderLeadMinutes(1), 15);
  CHECK_EQ(reminderLeadMinutes(2), 30);
  CHECK_EQ(reminderLeadMinutes(7), 0);

  CHECK(isValidMethod(2));
  CHECK(isValidMethod(3));
  CHECK(isValidMethod(13));
  CHECK(isValidMethod(0));
  CHECK(isValidMethod(23));
  CHECK(!isValidMethod(6));
  CHECK(!isValidMethod(-1));
  CHECK(!isValidMethod(24));
  CHECK(!isValidMethod(99));
}

static void testOffsets() {
  int16_t out[kPrayerCount];
  const int none[kPrayerCount] = {0, 0, 0, 0, 0, 0};
  applyOffsets(kDay, none, out);
  for (int i = 0; i < kPrayerCount; i++) CHECK_EQ(out[i], kDay[i]);

  const int adj[kPrayerCount] = {-5, 0, 2, 10, -30, 30};
  applyOffsets(kDay, adj, out);
  CHECK_EQ(out[FAJR], kDay[FAJR] - 5);
  CHECK_EQ(out[DHUHR], kDay[DHUHR] + 2);
  CHECK_EQ(out[MAGHRIB], kDay[MAGHRIB] - 30);
  CHECK_EQ(out[ISHA], kDay[ISHA] + 30);

  const int tooMuch[kPrayerCount] = {-99, 0, 0, 0, 0, 99};  // wird auf ±30 begrenzt
  applyOffsets(kDay, tooMuch, out);
  CHECK_EQ(out[FAJR], kDay[FAJR] - 30);
  CHECK_EQ(out[ISHA], kDay[ISHA] + 30);

  // Über Mitternacht und unbekannte Zeiten
  const int16_t edge[kPrayerCount] = {5, -1, 780, 960, 1020, 1435};
  const int wrap[kPrayerCount] = {-10, 5, 0, 0, 0, 10};
  applyOffsets(edge, wrap, out);
  CHECK_EQ(out[FAJR], 1435);
  CHECK_EQ(out[SHURUK], -1);
  CHECK_EQ(out[ISHA], 5);

  applyOffsets(NULL, wrap, out);  // Tag unbekannt
  for (int i = 0; i < kPrayerCount; i++) CHECK_EQ(out[i], -1);
}

// Monatswechsel: vorgeladener Folgemonat wird übernommen, Daten fremder Monate nie benutzt.
static void testMonthCache() {
  MonthTimes cur, next;
  fillMonth(cur, 2026, 2, 0);
  fillMonth(next, 2026, 3, 100);

  CHECK(timesForDate(cur, next, 2026, 2, 28) == cur.t[27]);
  CHECK(timesForDate(cur, next, 2026, 3, 1) == next.t[0]);   // morgen = Folgemonat
  CHECK(timesForDate(cur, next, 2026, 2, 29) == NULL);       // gibt es nicht
  CHECK(timesForDate(cur, next, 2026, 4, 1) == NULL);        // unbekannt -> nichts anzeigen
  CHECK(timesForDate(cur, next, 2025, 2, 1) == NULL);        // falsches Jahr

  CHECK(!promoteNextMonth(cur, next, 2026, 2));  // noch Februar
  CHECK(promoteNextMonth(cur, next, 2026, 3));
  CHECK(monthMatches(cur, 2026, 3));
  CHECK_EQ(cur.t[0][FAJR], kDay[FAJR] + 100);
  CHECK(!monthMatches(next, 2026, 3) && next.days == 0);

  // Regression: alter Monat (Februar, 28 Tage) darf am 31. März nicht benutzt werden
  MonthTimes feb, empty;
  fillMonth(feb, 2026, 2, 0);
  clearMonth(empty);
  CHECK(timesForDate(feb, empty, 2026, 3, 31) == NULL);
  CHECK(!promoteNextMonth(feb, empty, 2026, 3));
}

static void testDisplayTimes() {
  const int16_t tomorrow[kPrayerCount] = {331, 481, 781, 961, 1021, 1141};
  int out[kPrayerCount];

  displayTimes(kDay, tomorrow, 12 * 60, out);  // 12:00: Fajr/Shuruk vorbei
  CHECK_EQ(out[FAJR], 331);
  CHECK_EQ(out[SHURUK], 481);
  CHECK_EQ(out[DHUHR], 780);
  CHECK_EQ(out[ISHA], 1140);

  displayTimes(kDay, NULL, 12 * 60, out);  // morgen unbekannt -> heutige Zeit
  CHECK_EQ(out[FAJR], 330);

  displayTimes(NULL, tomorrow, 12 * 60, out);  // heute unbekannt
  CHECK_EQ(out[FAJR], -1);
}

static Scheduler makeScheduler(const int athan[kPrayerCount], const int reminder[kPrayerCount]) {
  Scheduler s;
  s.setTimes(kDay);
  s.setModes(athan, reminder);
  return s;
}

static void testAthanOncePerPrayer() {
  const int athan[kPrayerCount] = {1, 0, 1, 1, 1, 1};
  Scheduler s = makeScheduler(athan, kAllOn);
  int count = 0;
  for (long sec = hms(12, 55, 0); sec <= hms(13, 5, 0); sec++) {
    Events ev = s.update(100, sec);
    if (ev.athanDue >= 0) {
      count++;
      CHECK_EQ(ev.athanDue, DHUHR);
      CHECK_EQ(sec, hms(13, 0, 0));
    }
  }
  CHECK_EQ(count, 1);
}

// Regression: früher lief bei Shuruk der Athan, weil prayerAthanModes[1] = 1 war.
static void testNoAthanAtShurukEvenIfEnabled() {
  Scheduler s = makeScheduler(kAllOn, kAllOn);
  for (long sec = hms(7, 59, 0); sec <= hms(8, 2, 0); sec++) CHECK_EQ(s.update(100, sec).athanDue, -1);
}

static void testAthanDisabled() {
  const int athan[kPrayerCount] = {1, 0, 0, 1, 1, 1};  // Dhuhr aus
  Scheduler s = makeScheduler(athan, kAllOn);
  int count = 0;
  for (long sec = hms(12, 59, 0); sec <= hms(13, 2, 0); sec++)
    if (s.update(100, sec).athanDue >= 0) count++;
  CHECK_EQ(count, 0);
}

static void testReminderWindow15() {
  Scheduler s = makeScheduler(kAllOn, kAllOn);
  Events ev = s.update(100, hms(5, 14, 59));
  CHECK_EQ(ev.activeReminder, -1);

  ev = s.update(100, hms(5, 15, 0));
  CHECK_EQ(ev.activeReminder, FAJR);
  CHECK_EQ(ev.reminderStarted, FAJR);
  CHECK_EQ(ev.reminderSecondsLeft, 15 * 60);

  ev = s.update(100, hms(5, 15, 1));
  CHECK_EQ(ev.reminderStarted, -1);  // Ton nur einmal
  CHECK_EQ(ev.reminderSecondsLeft, 15 * 60 - 1);

  ev = s.update(100, hms(5, 30, 0));
  CHECK_EQ(ev.activeReminder, -1);
  CHECK_EQ(ev.athanDue, FAJR);
}

// Regression: Modus 2 (30 min) spielte den Ton, der Countdown kam aber erst bei 15 min.
static void testReminderWindow30() {
  const int reminder[kPrayerCount] = {1, 1, 2, 1, 1, 1};
  Scheduler s = makeScheduler(kAllOn, reminder);
  Events ev = s.update(100, hms(12, 29, 59));
  CHECK_EQ(ev.activeReminder, -1);
  ev = s.update(100, hms(12, 30, 0));
  CHECK_EQ(ev.reminderStarted, DHUHR);
  CHECK_EQ(ev.activeReminder, DHUHR);
  CHECK_EQ(ev.reminderSecondsLeft, 30 * 60);
}

// Regression: Countdown erschien auch bei ausgeschalteter Erinnerung.
static void testReminderDisabled() {
  const int reminder[kPrayerCount] = {1, 1, 0, 1, 1, 1};
  Scheduler s = makeScheduler(kAllOn, reminder);
  int active = 0;
  for (long sec = hms(12, 40, 0); sec < hms(13, 0, 0); sec++)
    if (s.update(100, sec).activeReminder == DHUHR) active++;
  CHECK_EQ(active, 0);
}

// Asr 16:00 und Maghrib 16:20 (Winter): Countdowns überlappen -> der nähere gewinnt.
static void testOverlappingReminders() {
  const int16_t winter[kPrayerCount] = {380, 470, 750, 960, 980, 1070};
  const int reminder[kPrayerCount] = {1, 1, 1, 1, 2, 1};  // Maghrib 30 min vorher
  Scheduler s;
  s.setTimes(winter);
  s.setModes(kAllOn, reminder);
  Events ev = s.update(1, hms(15, 55, 0));
  CHECK_EQ(ev.activeReminder, ASR);
  ev = s.update(1, hms(16, 0, 1));
  CHECK_EQ(ev.activeReminder, MAGHRIB);
}

static void testAthanAfterShortStall() {
  Scheduler s = makeScheduler(kAllOn, kAllOn);
  s.update(100, hms(12, 59, 55));
  CHECK_EQ(s.update(100, hms(13, 0, 7)).athanDue, DHUHR);
}

static void testUnknownTimes() {
  Scheduler s;
  s.setTimes(NULL);
  s.setModes(kAllOn, kAllOn);
  int events = 0;
  for (long sec = 0; sec < 86400; sec++) {
    Events ev = s.update(1, sec);
    if (ev.athanDue != -1 || ev.activeReminder != -1) events++;
  }
  CHECK_EQ(events, 0);
}

// Drei Tage Sekunde für Sekunde: jedes Ereignis genau einmal pro Tag.
static void testFullDaysSimulation() {
  const int athan[kPrayerCount] = {1, 0, 1, 1, 1, 1};
  Scheduler s = makeScheduler(athan, kAllOn);
  int athans[kPrayerCount] = {0};
  int reminders[kPrayerCount] = {0};
  for (long day = 1000; day < 1003; day++) {
    for (long sec = 0; sec < 86400; sec++) {
      Events ev = s.update(day, sec);
      if (ev.athanDue >= 0) athans[ev.athanDue]++;
      if (ev.reminderStarted >= 0) reminders[ev.reminderStarted]++;
    }
  }
  for (int i = 0; i < kPrayerCount; i++) {
    CHECK_EQ(reminders[i], 3);
    CHECK_EQ(athans[i], hasAthan(i) ? 3 : 0);
  }
}

// Neue Zeiten am Folgetag (wie nach dem Tageswechsel): Gebete feuern wieder.
static void testNewTimesNextDay() {
  const int16_t day2[kPrayerCount] = {331, 481, 780, 959, 1019, 1139};
  Scheduler s = makeScheduler(kAllOn, kAllOn);
  int dhuhr = 0;
  for (long sec = hms(12, 59, 0); sec < hms(13, 2, 0); sec++)
    if (s.update(1, sec).athanDue == DHUHR) dhuhr++;
  s.setTimes(day2);
  for (long sec = hms(12, 59, 0); sec < hms(13, 2, 0); sec++)
    if (s.update(2, sec).athanDue == DHUHR) dhuhr++;
  CHECK_EQ(dhuhr, 2);
}

int main() {
  struct {
    const char* name;
    void (*fn)();
  } tests[] = {
      {"Zeit parsen/formatieren", testParseAndFormat},
      {"Kalender (Schaltjahr, Monats-/Jahreswechsel)", testCalendar},
      {"API-URL mit Kodierung", testCalendarUrl},
      {"API-URL mit Koordinaten", testCoordinateUrl},
      {"Koordinaten & Zeitzone pruefen", testLocationChecks},
      {"Umlaute fuer das Display", testTransliteration},
      {"Minuten-Korrektur pro Gebet", testOffsets},
      {"Modi", testModes},
      {"Monats-Cache / Monatswechsel", testMonthCache},
      {"Angezeigte Zeiten (heute/morgen)", testDisplayTimes},
      {"Athan nur einmal", testAthanOncePerPrayer},
      {"Kein Athan bei Shuruk", testNoAthanAtShurukEvenIfEnabled},
      {"Athan abgeschaltet", testAthanDisabled},
      {"Erinnerung 15 min", testReminderWindow15},
      {"Erinnerung 30 min", testReminderWindow30},
      {"Erinnerung aus -> kein Countdown", testReminderDisabled},
      {"Ueberlappende Countdowns", testOverlappingReminders},
      {"Athan nach kurzer Blockade", testAthanAfterShortStall},
      {"Unbekannte Zeiten", testUnknownTimes},
      {"3 Tage Simulation", testFullDaysSimulation},
      {"Neue Zeiten am Folgetag", testNewTimesNextDay},
  };

  for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
    int before = failures;
    tests[i].fn();
    printf("%s %s\n", failures == before ? "[ OK ]" : "[FAIL]", tests[i].name);
  }
  printf("\n%d Pruefungen, %d Fehler\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
