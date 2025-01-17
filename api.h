#ifndef API_H
#define API_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// Funktionen
void fetchPrayerTimes(String& fajr, String& shuruk, String& dhuhr, String& asr, String& maghrib, String& isha, const String& apiUrl);
#endif
