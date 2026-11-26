// html.h
// Weboberfläche: http://athanclock.local (oder IP-Adresse)
//
//   GET  /               Einstellungsseite (web_page.h, aus dem Flash)
//   GET  /api/state      Uhrzeit, Gebetszeiten, Einstellungen, Systeminfos (JSON)
//   POST /api/settings   Töne, Lautstärke, Modi pro Gebet ändern (JSON, Felder optional)
//   POST /api/location   {"name": "...", "region": "...", "lat": 49.98, "lon": 8.28, "method": 2}
//                        -> Zeiten neu laden. Ohne "name" ändert sich nur die Methode.
//                        Koordinaten kommen aus der Ortssuche der Webseite (Open-Meteo).
//   POST /api/preview    {"kind": "athan"|"fajr"|"reminder", "tone": 0-5} -> Probehören
//   POST /api/stop       Wiedergabe stoppen
//   GET  /wifi           WLAN-Einrichtungsseite (im Hotspot auch unter /)
//   GET  /api/wifi/status, /api/wifi/scan; POST /api/wifi {"ssid","pass"}
//                        (im Normalbetrieb mit Header X-Device-Password)
//   GET/POST /update     Firmware-Upload (.bin), Benutzer "admin", Geräte-Passwort
//   ArduinoOTA           Port 8266, Geräte-Passwort (z. B. aus PlatformIO)
#ifndef HTML_H
#define HTML_H

#include <ESP8266WebServer.h>

#define WEB_HOSTNAME "athanclock"

extern ESP8266WebServer server;

void setupWebServer();   // in setup() aufrufen
void updateWebServer();  // in loop() aufrufen (Anfragen + mDNS)

#endif
