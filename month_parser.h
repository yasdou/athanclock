// month_parser.h
// Liest einen aladhan-Monatskalender Tag für Tag aus einem Datenstrom.
// Statt die ganze Antwort (~40 KB, gefiltert ~7 KB) auf einmal in ein
// JsonDocument zu laden, liegt immer nur ein Tag (~300 Bytes) im Speicher.
//
// Als Template geschrieben, damit derselbe Code auf dem ESP8266 (Arduino-Stream)
// und auf dem PC (Test-Reader, siehe test/test_month_parser.cpp) läuft.
#ifndef MONTH_PARSER_H
#define MONTH_PARSER_H

#include <ArduinoJson.h>
#include <string.h>

#include "prayer_logic.h"

namespace prayer {

// JSON-Schlüssel bei aladhan, gleiche Reihenfolge wie PrayerIndex
static const char* const kApiKeys[kPrayerCount] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};

// TStream braucht: read(), readBytes() (für ArduinoJson) sowie
// find(const char*) und findUntil(const char*, const char*) wie Arduinos Stream.
//
// Schreibt nur bei vollständigem Erfolg nach out; sonst bleibt out unverändert
// und *error beschreibt den Fehler. afterDay (optional) wird nach jedem Tag
// aufgerufen, z. B. um den Speicher zu messen.
template <typename TStream>
bool parseMonthStream(TStream& stream, int year, int month, MonthTimes& out, const char** error,
                      void (*afterDay)() = NULL) {
  const char* dummy;
  if (error == NULL) error = &dummy;
  *error = "";

  int expectedDays = daysInMonth(year, month);
  if (expectedDays <= 0) {
    *error = "ungueltiger Monat";
    return false;
  }

  if (!stream.find("\"data\":[")) {
    *error = "\"data\"-Array nicht gefunden";
    return false;
  }

  JsonDocument filter;
  for (int i = 0; i < kPrayerCount; i++) filter["timings"][kApiKeys[i]] = true;
  filter["meta"]["timezone"] = true;  // Zeitzone des Orts laut aladhan

  int16_t parsed[31][kPrayerCount];
  char timezone[sizeof(out.timezone)] = "";
  int day = 0;
  do {
    if (day >= expectedDays) {
      *error = "mehr Tage als erwartet";
      return false;
    }

    JsonDocument doc;  // nur ein Tag
    DeserializationError err = deserializeJson(doc, stream, DeserializationOption::Filter(filter));
    if (err) {
      *error = err.c_str();
      return false;
    }

    for (int i = 0; i < kPrayerCount; i++) {
      const char* value = doc["timings"][kApiKeys[i]];
      int minutes = parseHHMM(value);
      if (minutes < 0) {
        *error = "ungueltige oder fehlende Zeit";
        return false;
      }
      parsed[day][i] = (int16_t)minutes;
    }
    if (day == 0) {
      const char* tz = doc["meta"]["timezone"] | "";
      strncpy(timezone, tz, sizeof(timezone) - 1);
      timezone[sizeof(timezone) - 1] = '\0';
    }
    day++;
    if (afterDay) afterDay();
  } while (stream.findUntil(",", "]"));  // weiter bis zum Ende des Arrays

  if (day != expectedDays) {
    *error = "weniger Tage als erwartet";
    return false;
  }

  clearMonth(out);
  out.year = year;
  out.month = month;
  out.days = expectedDays;
  memcpy(out.t, parsed, sizeof(int16_t) * kPrayerCount * expectedDays);
  memcpy(out.timezone, timezone, sizeof(out.timezone));
  return true;
}

}  // namespace prayer

#endif
