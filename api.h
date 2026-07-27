// api.h
#ifndef API_H
#define API_H

#include <Arduino.h>

struct PrayerDay {
    String fajr;
    String shuruk;
    String dhuhr;
    String asr;
    String maghrib;
    String isha;
};

bool fetchMonthlyPrayerTimes(PrayerDay monthlyPrayerTimes[], int& daysInMonth, const String& apiUrl);

#endif