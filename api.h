// api.h
#ifndef API_H
#define API_H

#include "prayer_logic.h"

// Lädt einen Monatskalender von aladhan nach out.
// Gibt nur bei vollständig gültigen Daten true zurück; sonst bleibt out unverändert.
bool fetchMonthlyPrayerTimes(const char* apiUrl, int year, int month, prayer::MonthTimes& out);

#endif
