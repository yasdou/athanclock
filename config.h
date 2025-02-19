// config.h
#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>  // String und andere Arduino-Typen


// WiFi-Einstellungen
extern const char* ssid;
extern const char* password;

// Variablen für das Datum
extern int currentDay, currentMonth, currentYear;
extern String apiUrl;  // Die dynamische URL

// APP Einstellungen
extern int prayerAthanModes[6];
extern int prayerReminderModes[6];
extern String selectedCity ;                  
extern String athanTone;            
extern String reminderTone; 

extern String fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime;


#endif
