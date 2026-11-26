#include "prayer_logic.h"

#include <stdio.h>
#include <string.h>

namespace prayer {

const char* const kNames[kPrayerCount] = {"Fajr", "Shuruk", "Dhuhr", "Asr", "Maghrib", "Isha"};

static const long kSecondsPerDay = 86400L;
static const long kNeverFired = -1000000L;

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

// ---------------------------------------------------------------------------
// Zeit-Helfer

int parseHHMM(const char* s) {
  if (s == NULL) return -1;
  for (int i = 0; i < 5; i++) {
    if (s[i] == '\0') return -1;
  }
  if (!isDigit(s[0]) || !isDigit(s[1]) || s[2] != ':' || !isDigit(s[3]) || !isDigit(s[4])) return -1;
  if (isDigit(s[5])) return -1;  // z. B. "12:345"
  int h = (s[0] - '0') * 10 + (s[1] - '0');
  int m = (s[3] - '0') * 10 + (s[4] - '0');
  if (h > 23 || m > 59) return -1;
  return h * 60 + m;
}

void formatHHMM(int minutes, char* buf, size_t len) {
  if (minutes < 0 || minutes >= 24 * 60) {
    snprintf(buf, len, "--:--");
    return;
  }
  snprintf(buf, len, "%02d:%02d", minutes / 60, minutes % 60);
}

void formatCountdown(long seconds, char* buf, size_t len) {
  if (seconds < 0) seconds = 0;
  long m = seconds / 60;
  if (m > 99) m = 99;
  snprintf(buf, len, "%02ld:%02ld", m, seconds % 60);
}

long secondsUntil(long nowSec, int prayerMin) {
  long diff = (prayerMin * 60L - nowSec) % kSecondsPerDay;
  if (diff < 0) diff += kSecondsPerDay;
  return diff;
}

// ---------------------------------------------------------------------------
// Kalender-Helfer

// Algorithmus nach Howard Hinnant ("days_from_civil").
long daysFromCivil(int year, int month, int day) {
  long y = year - (month <= 2 ? 1 : 0);
  long era = (y >= 0 ? y : y - 399) / 400;
  long yoe = y - era * 400;
  long mp = month + (month > 2 ? -3 : 9);
  long doy = (153 * mp + 2) / 5 + day - 1;
  long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

int daysInMonth(int year, int month) {
  switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      return 31;
    case 4: case 6: case 9: case 11:
      return 30;
    case 2: {
      bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      return leap ? 29 : 28;
    }
    default:
      return 0;
  }
}

void addMonths(int year, int month, int offset, int& outYear, int& outMonth) {
  int total = year * 12 + (month - 1) + offset;
  outYear = total / 12;
  outMonth = total % 12 + 1;
}

void nextDay(int year, int month, int day, int& outYear, int& outMonth, int& outDay) {
  if (day < daysInMonth(year, month)) {
    outYear = year;
    outMonth = month;
    outDay = day + 1;
    return;
  }
  addMonths(year, month, 1, outYear, outMonth);
  outDay = 1;
}

static bool isUnreserved(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || isDigit(c) || c == '-' || c == '_' ||
         c == '.' || c == '~';
}

static bool appendEncoded(char* buf, size_t len, size_t& pos, const char* s) {
  static const char hex[] = "0123456789ABCDEF";
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if (isUnreserved((char)c)) {
      if (pos + 1 >= len) return false;
      buf[pos++] = (char)c;
    } else {
      if (pos + 3 >= len) return false;
      buf[pos++] = '%';
      buf[pos++] = hex[c >> 4];
      buf[pos++] = hex[c & 0x0F];
    }
  }
  buf[pos] = '\0';
  return true;
}

static bool appendRaw(char* buf, size_t len, size_t& pos, const char* s) {
  for (; *s; s++) {
    if (pos + 1 >= len) return false;
    buf[pos++] = *s;
  }
  buf[pos] = '\0';
  return true;
}

bool buildCalendarUrl(char* buf, size_t len, int year, int month, const char* city, const char* country,
                      int method, int school) {
  if (buf == NULL || len == 0 || city == NULL || country == NULL) return false;
  buf[0] = '\0';
  char part[80];
  int n = snprintf(part, sizeof(part), "https://api.aladhan.com/v1/calendarByCity/%d/%d?city=", year, month);
  if (n < 0 || (size_t)n >= sizeof(part)) return false;
  size_t pos = 0;
  if (!appendRaw(buf, len, pos, part)) return false;
  if (!appendEncoded(buf, len, pos, city)) return false;
  if (!appendRaw(buf, len, pos, "&country=")) return false;
  if (!appendEncoded(buf, len, pos, country)) return false;
  n = snprintf(part, sizeof(part), "&method=%d&school=%d", method, school == 1 ? 1 : 0);
  if (n < 0 || (size_t)n >= sizeof(part)) return false;
  return appendRaw(buf, len, pos, part);
}

void applyOffsets(const int16_t* in, const int offsets[kPrayerCount], int16_t out[kPrayerCount]) {
  for (int i = 0; i < kPrayerCount; i++) {
    int t = in ? in[i] : -1;
    if (t < 0) {
      out[i] = -1;
      continue;
    }
    int off = offsets ? offsets[i] : 0;
    if (off > kMaxOffsetMinutes) off = kMaxOffsetMinutes;
    if (off < -kMaxOffsetMinutes) off = -kMaxOffsetMinutes;
    out[i] = (int16_t)(((t + off) % 1440 + 1440) % 1440);
  }
}

// Formatiert eine Koordinate mit 5 Nachkommastellen ohne %f (printf-Gleitkomma
// ist auf Mikrocontrollern nicht überall verfügbar).
static void formatCoordinate(double value, char* buf, size_t len) {
  bool negative = value < 0;
  double absValue = negative ? -value : value;
  long scaled = (long)(absValue * 100000.0 + 0.5);
  snprintf(buf, len, "%s%ld.%05ld", (negative && scaled != 0) ? "-" : "", scaled / 100000L, scaled % 100000L);
}

bool buildCalendarUrlByCoords(char* buf, size_t len, int year, int month, double latitude, double longitude,
                              int method, int school) {
  if (buf == NULL || len == 0 || !isValidCoordinate(latitude, longitude)) return false;
  char lat[16], lon[16];
  formatCoordinate(latitude, lat, sizeof(lat));
  formatCoordinate(longitude, lon, sizeof(lon));
  int n = snprintf(buf, len,
                   "https://api.aladhan.com/v1/calendar/%d/%d?latitude=%s&longitude=%s&method=%d&school=%d", year,
                   month, lat, lon, method, school == 1 ? 1 : 0);
  return n > 0 && (size_t)n < len;
}

// ---------------------------------------------------------------------------
// Standort-Prüfungen

bool isValidCoordinate(double latitude, double longitude) {
  // NaN ist nie >= oder <= -> wird hier automatisch abgelehnt
  return latitude >= -90.0 && latitude <= 90.0 && longitude >= -180.0 && longitude <= 180.0;
}

static const char* const kCentralEuropeanZones[] = {
    "Europe/Berlin",    "Europe/Busingen",  "Europe/Vienna",     "Europe/Zurich",    "Europe/Vaduz",
    "Europe/Paris",     "Europe/Monaco",    "Europe/Luxembourg", "Europe/Brussels",  "Europe/Amsterdam",
    "Europe/Copenhagen", "Europe/Oslo",     "Europe/Stockholm",  "Europe/Rome",      "Europe/Vatican",
    "Europe/San_Marino", "Europe/Malta",    "Europe/Madrid",     "Europe/Andorra",   "Europe/Gibraltar",
    "Europe/Prague",    "Europe/Bratislava", "Europe/Warsaw",    "Europe/Budapest",  "Europe/Ljubljana",
    "Europe/Zagreb",    "Europe/Sarajevo",  "Europe/Belgrade",   "Europe/Podgorica", "Europe/Skopje",
    "Europe/Tirane",    "Arctic/Longyearbyen", "Africa/Ceuta",   "CET"};

bool isCentralEuropeanTimezone(const char* tz) {
  if (tz == NULL) return false;
  for (size_t i = 0; i < sizeof(kCentralEuropeanZones) / sizeof(kCentralEuropeanZones[0]); i++) {
    if (strcmp(tz, kCentralEuropeanZones[i]) == 0) return true;
  }
  return false;
}

void transliterateForDisplay(const char* in, char* out, size_t len) {
  if (out == NULL || len == 0) return;
  size_t pos = 0;
  out[0] = '\0';
  if (in == NULL) return;

  // Zweibyte-UTF-8-Zeichen (Latin-1-Bereich) -> Ersatztext
  struct Map {
    unsigned char b2;
    const char* rep;
  };
  static const Map kMap[] = {
      {0x84, "Ae"}, {0x96, "Oe"}, {0x9C, "Ue"}, {0xA4, "ae"}, {0xB6, "oe"}, {0xBC, "ue"}, {0x9F, "ss"},
      {0xA0, "a"},  {0xA1, "a"},  {0xA2, "a"},  {0xA7, "c"},  {0xA8, "e"},  {0xA9, "e"},  {0xAA, "e"},
      {0xAB, "e"},  {0xAD, "i"},  {0xAE, "i"},  {0xB3, "o"},  {0xB4, "o"},  {0xB9, "u"},  {0xBA, "u"},
      {0x80, "A"},  {0x81, "A"},  {0x87, "C"},  {0x88, "E"},  {0x89, "E"},  {0x93, "O"},  {0xB1, "n"},
  };

  const unsigned char* p = (const unsigned char*)in;
  while (*p && pos + 1 < len) {
    const char* rep = NULL;
    char single[2] = {0, 0};
    if (*p < 0x80) {
      single[0] = (char)*p;
      rep = single;
      p++;
    } else if (*p == 0xC3 && p[1] >= 0x80 && p[1] <= 0xBF) {
      for (size_t i = 0; i < sizeof(kMap) / sizeof(kMap[0]); i++) {
        if (kMap[i].b2 == p[1]) {
          rep = kMap[i].rep;
          break;
        }
      }
      if (rep == NULL) rep = "?";
      p += 2;
    } else {
      // anderes Mehrbyte-Zeichen: komplett überspringen, '?' ausgeben
      p++;
      while ((*p & 0xC0) == 0x80) p++;
      rep = "?";
    }
    for (; *rep && pos + 1 < len; rep++) out[pos++] = *rep;
  }
  out[pos] = '\0';
}

// ---------------------------------------------------------------------------
// Einstellungen

bool hasAthan(int index) { return index >= 0 && index < kPrayerCount && index != SHURUK; }

int reminderLeadMinutes(int mode) {
  if (mode == 1) return 15;
  if (mode == 2) return 30;
  return 0;
}

bool isValidMethod(int method) { return method >= 0 && method <= 23 && method != 6; }

// ---------------------------------------------------------------------------
// Monatsdaten

void clearMonth(MonthTimes& m) {
  m.year = 0;
  m.month = 0;
  m.days = 0;
  m.timezone[0] = '\0';
  for (int d = 0; d < 31; d++)
    for (int i = 0; i < kPrayerCount; i++) m.t[d][i] = -1;
}

bool monthMatches(const MonthTimes& m, int year, int month) {
  return m.days > 0 && m.year == year && m.month == month;
}

bool promoteNextMonth(MonthTimes& current, MonthTimes& next, int year, int month) {
  if (monthMatches(current, year, month)) return false;
  if (!monthMatches(next, year, month)) return false;
  current = next;
  clearMonth(next);
  return true;
}

const int16_t* timesForDate(const MonthTimes& current, const MonthTimes& next, int year, int month, int day) {
  if (day < 1) return NULL;
  if (monthMatches(current, year, month) && day <= current.days) return current.t[day - 1];
  if (monthMatches(next, year, month) && day <= next.days) return next.t[day - 1];
  return NULL;
}

void displayTimes(const int16_t* today, const int16_t* tomorrow, int nowMinute, int out[kPrayerCount]) {
  for (int i = 0; i < kPrayerCount; i++) {
    int t = today ? today[i] : -1;
    if (t >= 0 && nowMinute >= t && tomorrow != NULL && tomorrow[i] >= 0) t = tomorrow[i];
    out[i] = t;
  }
}

// ---------------------------------------------------------------------------
// Scheduler

Scheduler::Scheduler() {
  for (int i = 0; i < kPrayerCount; i++) {
    times_[i] = -1;
    athanModes_[i] = hasAthan(i) ? 1 : 0;
    reminderModes_[i] = 1;
    reminderFiredDay_[i] = kNeverFired;
    athanFiredDay_[i] = kNeverFired;
  }
}

void Scheduler::setTimes(const int16_t* minutes) {
  for (int i = 0; i < kPrayerCount; i++) times_[i] = minutes ? minutes[i] : -1;
}

void Scheduler::setModes(const int athanModes[kPrayerCount], const int reminderModes[kPrayerCount]) {
  for (int i = 0; i < kPrayerCount; i++) {
    athanModes_[i] = athanModes[i];
    reminderModes_[i] = reminderModes[i];
  }
}

Events Scheduler::update(long dayKey, long nowSec) {
  Events ev = {-1, -1, -1, 0};
  long bestLeft = kSecondsPerDay;

  for (int i = 0; i < kPrayerCount; i++) {
    if (times_[i] < 0) continue;
    long prayerSec = times_[i] * 60L;

    // Countdown-Fenster: (Gebet - lead, Gebet]
    long lead = reminderLeadMinutes(reminderModes_[i]) * 60L;
    long until = secondsUntil(nowSec, times_[i]);
    if (lead > 0 && until > 0 && until <= lead) {
      // Liegt das Gebet schon hinter Mitternacht, gehört es zum nächsten Tag.
      long occurrenceDay = (prayerSec > nowSec) ? dayKey : dayKey + 1;
      if (until < bestLeft) {
        bestLeft = until;
        ev.activeReminder = i;
        ev.reminderSecondsLeft = until;
      }
      if (reminderFiredDay_[i] != occurrenceDay) {
        reminderFiredDay_[i] = occurrenceDay;
        if (ev.reminderStarted < 0) ev.reminderStarted = i;
      }
    }

    // Athan: genau einmal innerhalb von kAthanWindowSec nach der Gebetszeit.
    if (hasAthan(i) && athanModes_[i] != 0) {
      long elapsed = (nowSec - prayerSec) % kSecondsPerDay;
      if (elapsed < 0) elapsed += kSecondsPerDay;
      if (elapsed < kAthanWindowSec) {
        long occurrenceDay = (nowSec >= prayerSec) ? dayKey : dayKey - 1;
        if (athanFiredDay_[i] != occurrenceDay) {
          athanFiredDay_[i] = occurrenceDay;
          if (ev.athanDue < 0) ev.athanDue = i;
        }
      }
    }
  }
  return ev;
}

}  // namespace prayer
