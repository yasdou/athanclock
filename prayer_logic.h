// prayer_logic.h
// Reine Zeit-/Gebetslogik ohne Arduino-Abhängigkeiten.
// Dadurch lässt sich alles hier auf dem PC testen (siehe test/).
#ifndef PRAYER_LOGIC_H
#define PRAYER_LOGIC_H

#include <stddef.h>
#include <stdint.h>

namespace prayer {

const int kPrayerCount = 6;
enum PrayerIndex { FAJR = 0, SHURUK, DHUHR, ASR, MAGHRIB, ISHA };

extern const char* const kNames[kPrayerCount];

// Wie lange (Sekunden) nach der Gebetszeit der Athan noch ausgelöst werden darf,
// falls die Schleife kurz blockiert war (z. B. durch einen HTTPS-Abruf).
const long kAthanWindowSec = 60;

// ---- Zeit-Helfer ----

// "HH:MM" -> Minuten seit Mitternacht. Ein Suffix wie " (CET)" wird ignoriert.
// Gibt -1 zurück, wenn der Text ungültig ist.
int parseHHMM(const char* s);

// Minuten seit Mitternacht -> "HH:MM" (oder "--:--" bei ungültig). buf >= 6 Zeichen.
void formatHHMM(int minutes, char* buf, size_t len);

// Sekunden -> "MM:SS" für den Countdown. buf >= 6 Zeichen.
void formatCountdown(long seconds, char* buf, size_t len);

// Sekunden von nowSec (0..86399) bis prayerMin, mit Umbruch über Mitternacht (0..86399).
long secondsUntil(long nowSec, int prayerMin);

// ---- Kalender-Helfer ----

long daysFromCivil(int year, int month, int day);  // Tage seit 1970-01-01
int daysInMonth(int year, int month);
void addMonths(int year, int month, int offset, int& outYear, int& outMonth);
void nextDay(int year, int month, int day, int& outYear, int& outMonth, int& outDay);

// Baut die aladhan-URL für einen Monatskalender. Stadt/Land werden URL-kodiert.
// school: Asr-Berechnung, 0 = Standard (Schafi'i/Maliki/Hanbali), 1 = Hanafi.
// Gibt false zurück, wenn der Puffer zu klein ist.
bool buildCalendarUrl(char* buf, size_t len, int year, int month, const char* city, const char* country,
                      int method, int school);

// Wie oben, aber über Koordinaten: eindeutig, ohne Namenssuche bei aladhan.
bool buildCalendarUrlByCoords(char* buf, size_t len, int year, int month, double latitude, double longitude,
                              int method, int school);

// Korrektur in Minuten pro Gebet (damit die Zeiten zur eigenen Moschee passen).
const int kMaxOffsetMinutes = 30;
// Wendet die Korrekturen an. Unbekannte Zeiten (-1) bleiben -1; Ergebnis bleibt im Tag (0..1439).
void applyOffsets(const int16_t* in, const int offsets[kPrayerCount], int16_t out[kPrayerCount]);

// ---- Standort-Prüfungen ----

bool isValidCoordinate(double latitude, double longitude);

// Zeitzonen mit denselben Regeln wie die Uhr (MEZ/MESZ, EU-Sommerzeit).
// Orte in anderen Zeitzonen würden in falscher Ortszeit angezeigt.
bool isCentralEuropeanTimezone(const char* tz);

// UTF-8 -> ASCII für das Display (dessen Schrift kennt keine Umlaute):
// ä->ae, ö->oe, ü->ue, ß->ss, é->e ...; unbekannte Zeichen -> '?'.
void transliterateForDisplay(const char* in, char* out, size_t len);

// ---- Einstellungen ----

// Shuruk (Sonnenaufgang) ist kein Gebet -> nie ein Athan.
bool hasAthan(int index);

// Erinnerungsmodus -> Vorlaufzeit in Minuten (0 = aus, 1 = 15 min, 2 = 30 min).
int reminderLeadMinutes(int mode);

// Gültige aladhan-Berechnungsmethoden (0-23 ohne 6; 99 "Custom" braucht Zusatzparameter).
bool isValidMethod(int method);

// ---- Monatsdaten ----

struct MonthTimes {
  int year;   // 0 = leer
  int month;
  int days;
  int16_t t[31][kPrayerCount];  // Minuten seit Mitternacht
  char timezone[40];            // laut aladhan, z. B. "Europe/Berlin" ("" = unbekannt)
};

void clearMonth(MonthTimes& m);
bool monthMatches(const MonthTimes& m, int year, int month);

// Ist der vorgeladene Folgemonat jetzt der aktuelle Monat, wird er übernommen
// (kein erneuter Abruf am 1. nötig). Gibt true zurück, wenn getauscht wurde.
bool promoteNextMonth(MonthTimes& current, MonthTimes& next, int year, int month);

// Zeiten für ein Datum aus current oder next. NULL, wenn unbekannt.
const int16_t* timesForDate(const MonthTimes& current, const MonthTimes& next, int year, int month, int day);

// Anzeige: Für bereits vergangene Gebete wird die Zeit von morgen gezeigt
// (falls bekannt). Fehlende Werte werden -1.
void displayTimes(const int16_t* today, const int16_t* tomorrow, int nowMinute, int out[kPrayerCount]);

// ---- Scheduler ----

struct Events {
  int reminderStarted;       // Index, dessen Erinnerung in diesem Tick beginnt, sonst -1
  int athanDue;              // Index, dessen Athan jetzt fällig ist, sonst -1
  int activeReminder;        // Index des aktuell laufenden Countdowns, sonst -1
  long reminderSecondsLeft;  // Restsekunden für activeReminder
};

// Entscheidet pro Sekunde, ob Erinnerung/Athan fällig sind.
// Jedes Ereignis wird pro Gebet und Tag genau einmal gemeldet.
class Scheduler {
 public:
  Scheduler();
  void setTimes(const int16_t* minutes);  // NULL = Zeiten unbekannt
  void setModes(const int athanModes[kPrayerCount], const int reminderModes[kPrayerCount]);
  // dayKey: daysFromCivil(heute), nowSec: Sekunden seit Mitternacht (Ortszeit)
  Events update(long dayKey, long nowSec);

 private:
  int times_[kPrayerCount];
  int athanModes_[kPrayerCount];
  int reminderModes_[kPrayerCount];
  long reminderFiredDay_[kPrayerCount];
  long athanFiredDay_[kPrayerCount];
};

}  // namespace prayer

#endif
