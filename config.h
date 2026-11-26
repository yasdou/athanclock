// config.h
#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>
#include <time.h>

// Bei jedem Release erhöhen (wird auf Webseite und im Menü "Info" angezeigt)
#define FIRMWARE_VERSION "0.3.0"

// WiFi-Einstellungen (definiert in config.cpp, nicht in Git)
extern const char* ssid;
extern const char* password;

// Einstellungen (definiert in AthanClock.ino, gespeichert über settings.cpp)
extern int prayerAthanModes[6];     // 0 = aus, 1 = an
extern int prayerReminderModes[6];  // 0 = aus, 1 = 15 min, 2 = 30 min
// Standort. Mit Koordinaten werden die Zeiten eindeutig über latitude/longitude
// geladen; ohne (alte Einstellungen) über den Stadtnamen.
struct Location {
  String name;    // z. B. "München" (UTF-8, für Webseite; Display nutzt Umschrift)
  String region;  // z. B. "Bayern" (optional)
  double latitude;
  double longitude;
  bool hasCoords;
};

extern Location currentLocation;
extern String athanTone;
extern String reminderTone;
extern String fajrTone;  // eigener Athan für Fajr (Ordner 03 auf der SD-Karte)
extern int fajrVolume;   // leisere Lautstärke für Fajr-Athan und -Erinnerung (0-30)
extern int calcMethod;        // aladhan-Berechnungsmethode
extern int asrSchool;         // 0 = Standard (Schafi'i/Maliki/Hanbali), 1 = Hanafi
extern int prayerOffsets[6];  // Korrektur in Minuten pro Gebet (-30..30)

// Hilfsfunktionen aus AthanClock.ino
bool readLocalTime(struct tm& t);  // false, solange NTP nicht synchronisiert ist
void requestScreenRedraw();        // Startbildschirm beim nächsten Durchlauf neu zeichnen
// Lädt die Zeiten für Ort + Methode + Asr-Schule; übernimmt alles nur bei Erfolg.
// Bei Fehlschlag steht in *error (falls angegeben) eine kurze Begründung.
bool changeLocation(const Location& location, int method, int school, String* error = nullptr);
// Heutige Zeiten und nächstes Vorkommen jedes Gebets (Minuten seit Mitternacht, -1 = unbekannt)
void getPrayerOverview(int today[6], int upcoming[6]);

// aus settings.cpp
void markSettingsDirty();

#endif
