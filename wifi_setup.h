// wifi_setup.h
// WLAN-Verbindung und Einrichtung ohne Code-Änderung:
//
// - Zugangsdaten liegen in LittleFS (/wifi.json). Fehlen sie, werden die Werte
//   aus config.cpp genutzt (praktisch für die Entwicklung).
// - Klappt keine Verbindung (oder Taste UP beim Einschalten gedrückt), öffnet die
//   Uhr den Hotspot "AthanClock-XXXX" mit Einrichtungsseite (Captive Portal).
// - Im Einrichtungsmodus versucht die Uhr im Hintergrund weiter, sich mit dem
//   gespeicherten WLAN zu verbinden (z. B. wenn der Router nach einem Stromausfall
//   langsamer startet) und beendet den Hotspot dann von selbst.
// - Das Geräte-Passwort (Hotspot, Firmware-Update, WLAN ändern) wird beim ersten
//   Start zufällig erzeugt und gespeichert. Es steht auf dem Display.
#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Arduino.h>

void initDeviceIdentity();  // nach loadSettings() (LittleFS) aufrufen
void loadWifiCredentials();
bool hasWifiCredentials();
bool connectWifi(unsigned long timeoutMs);  // mit gespeicherten Daten, blockierend

void startSetupPortal();
bool isSetupMode();
void updateWifi();  // in loop(): DNS im Einrichtungsmodus, Reconnect im Normalbetrieb

const char* setupApName();
const char* devicePassword();
String savedWifiSsid();

// Aus der Einrichtungsseite: verbinden, bei Erfolg speichern. ipOut = neue IP.
bool trySetupConnect(const String& ssid, const String& pass, String* ipOut);
// Aus dem Normalbetrieb: neue Daten speichern und neu starten.
bool saveWifiCredentials(const String& ssid, const String& pass);
void scheduleRestart(unsigned long delayMs);

#endif
