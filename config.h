// config.h
#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>  // String und andere Arduino-Typen


// WiFi-Einstellungen
extern const char* ssid;
extern const char* password;

// APP Einstellungen
extern int prayerAthanModes[6];
extern int prayerReminderModes[6];
extern String selectedCity ;                  
extern String athanTone;            
extern String reminderTone; 

#endif
