#ifndef API_H
#define API_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// Funktionen
void fetchPrayerTimes(String& fajr, String& dhuhr, String& asr, String& maghrib, String& isha, String apiUrl);

#endif
