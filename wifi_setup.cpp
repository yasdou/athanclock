#include "wifi_setup.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "config.h"
#include "html.h"

static const char* kWifiFile = "/wifi.json";
static const char* kWifiTmp = "/wifi.tmp";
static const char* kDeviceFile = "/device.json";
static const IPAddress kApIp(192, 168, 4, 1);

static const unsigned long kStaRetryMs = 60UL * 1000UL;       // Hintergrund-Versuch im Einrichtungsmodus
static const unsigned long kStaAttemptMs = 15UL * 1000UL;     // so lange darf ein Versuch dauern
static const unsigned long kReconnectMs = 15UL * 1000UL;      // Normalbetrieb: neuer Versuch alle 15 s
static const unsigned long kPortalAfterMs = 5UL * 60UL * 1000UL;  // 5 min ohne WLAN -> Hotspot zusätzlich

static String wifiSsid, wifiPass;
static String devPassword;
static char apName[24] = "AthanClock";

static DNSServer dns;
static bool setupMode = false;
static bool staAttemptActive = false;
static unsigned long staAttemptStart = 0;
static unsigned long lastStaAttempt = 0;
static unsigned long leaveSetupAt = 0;
static unsigned long restartAt = 0;
static unsigned long disconnectedSince = 0;
static unsigned long lastReconnect = 0;

// ---------------------------------------------------------------------------
// Dateien

static bool writeJsonFile(const char* path, const char* tmp, const JsonDocument& doc) {
  File f = LittleFS.open(tmp, "w");
  if (!f) return false;
  size_t written = serializeJson(doc, f);
  f.close();
  if (written == 0) return false;
  LittleFS.remove(path);
  return LittleFS.rename(tmp, path);
}

void initDeviceIdentity() {
  snprintf(apName, sizeof(apName), "AthanClock-%04X", (unsigned)(ESP.getChipId() & 0xFFFF));

  LittleFS.begin();
  File f = LittleFS.open(kDeviceFile, "r");
  if (f) {
    JsonDocument doc;
    if (!deserializeJson(doc, f)) devPassword = doc["password"] | "";
    f.close();
  }
  if (devPassword.length() < 8) {
    // Ohne leicht verwechselbare Zeichen (0/O, 1/l/I), damit man es vom Display abtippen kann
    static const char alphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";
    char pw[9];
    for (int i = 0; i < 8; i++) pw[i] = alphabet[ESP.random() % (sizeof(alphabet) - 1)];
    pw[8] = '\0';
    devPassword = pw;
    JsonDocument doc;
    doc["password"] = devPassword;
    if (!writeJsonFile(kDeviceFile, "/device.tmp", doc)) Serial.println("Geraete-Passwort konnte nicht gespeichert werden.");
  }
  Serial.printf("Geraet: %s, Passwort: %s\n", apName, devPassword.c_str());
}

void loadWifiCredentials() {
  File f = LittleFS.open(kWifiFile, "r");
  if (f) {
    JsonDocument doc;
    if (!deserializeJson(doc, f)) {
      wifiSsid = doc["ssid"] | "";
      wifiPass = doc["pass"] | "";
    }
    f.close();
  }
  if (wifiSsid.length() == 0 && ssid != nullptr && strlen(ssid) > 0) {
    // Fallback: Zugangsdaten aus config.cpp
    wifiSsid = ssid;
    wifiPass = password ? password : "";
  }
  Serial.print("WLAN: ");
  Serial.println(wifiSsid.length() ? wifiSsid : String("(keins gespeichert)"));
}

bool saveWifiCredentials(const String& newSsid, const String& newPass) {
  JsonDocument doc;
  doc["ssid"] = newSsid;
  doc["pass"] = newPass;
  if (!writeJsonFile(kWifiFile, kWifiTmp, doc)) return false;
  wifiSsid = newSsid;
  wifiPass = newPass;
  return true;
}

bool hasWifiCredentials() { return wifiSsid.length() > 0; }
String savedWifiSsid() { return wifiSsid; }
const char* setupApName() { return apName; }
const char* devicePassword() { return devPassword.c_str(); }

void scheduleRestart(unsigned long delayMs) {
  restartAt = millis() + delayMs;
  if (restartAt == 0) restartAt = 1;
}

// ---------------------------------------------------------------------------
// Verbindung

bool connectWifi(unsigned long timeoutMs) {
  if (!hasWifiCredentials()) return false;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(WEB_HOSTNAME);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  Serial.print("Verbinde mit WLAN");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  bool ok = WiFi.status() == WL_CONNECTED;
  if (ok) {
    Serial.print("IP Adresse: ");
    Serial.println(WiFi.localIP());
  }
  return ok;
}

void startSetupPortal() {
  if (setupMode) return;
  setupMode = true;
  leaveSetupAt = 0;
  WiFi.persistent(false);
  // Wichtig: laufende Verbindungsversuche zum alten WLAN stoppen. Sonst sucht der
  // ESP8266 im Hintergrund weiter, wechselt dabei ständig den Funkkanal und der
  // Hotspot ist zwar sichtbar, aber Seiten laden nicht.
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(kApIp, kApIp, IPAddress(255, 255, 255, 0));
  bool apOk = WiFi.softAP(apName, devPassword.c_str());
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  bool dnsOk = dns.start(53, "*", kApIp);  // alle Namen -> Uhr (Captive Portal)
  Serial.printf("Hotspot %s (%s), DNS %s, IP %s\n", apName, apOk ? "ok" : "FEHLER", dnsOk ? "ok" : "FEHLER",
                WiFi.softAPIP().toString().c_str());
  WiFi.scanNetworks(true);  // Netzwerkliste schon mal im Hintergrund suchen
  staAttemptActive = false;
  lastStaAttempt = millis();
  Serial.printf("Einrichtungsmodus: Hotspot %s, http://192.168.4.1\n", apName);
  requestScreenRedraw();
}

static void stopSetupPortal() {
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  setupMode = false;
  staAttemptActive = false;
  disconnectedSince = 0;
  Serial.println("Einrichtungsmodus beendet, WLAN verbunden.");
  requestScreenRedraw();
}

bool isSetupMode() { return setupMode; }

bool trySetupConnect(const String& newSsid, const String& newPass, String* ipOut) {
  Serial.print("Teste WLAN: ");
  Serial.println(newSsid);
  WiFi.disconnect(false);
  WiFi.begin(newSsid.c_str(), newPass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
    delay(100);
    if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) break;
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(false);
    return false;
  }
  if (!saveWifiCredentials(newSsid, newPass)) Serial.println("WLAN-Daten konnten nicht gespeichert werden.");
  if (ipOut) *ipOut = WiFi.localIP().toString();
  // Hotspot noch kurz offen lassen, damit die Seite den Erfolg anzeigen kann
  leaveSetupAt = millis() + 10000UL;
  return true;
}

void updateWifi() {
  unsigned long now = millis();
  if (restartAt != 0 && (long)(now - restartAt) >= 0) {
    Serial.println("Neustart...");
    delay(100);
    ESP.restart();
  }

  if (setupMode) {
    dns.processNextRequest();

    if (WiFi.status() == WL_CONNECTED) {
      if (leaveSetupAt == 0 || (long)(now - leaveSetupAt) >= 0) stopSetupPortal();
      return;
    }

    // Im Hintergrund das gespeicherte WLAN erneut versuchen, aber nur, wenn gerade
    // niemand mit dem Hotspot verbunden ist (die Suche stört den Hotspot kurz).
    if (staAttemptActive && now - staAttemptStart > kStaAttemptMs) {
      WiFi.disconnect(false);
      staAttemptActive = false;
    }
    if (!staAttemptActive && hasWifiCredentials() && WiFi.softAPgetStationNum() == 0 &&
        now - lastStaAttempt > kStaRetryMs) {
      lastStaAttempt = now;
      staAttemptStart = now;
      staAttemptActive = true;
      WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    }
    return;
  }

  // Normalbetrieb
  if (WiFi.status() == WL_CONNECTED) {
    disconnectedSince = 0;
    return;
  }
  if (disconnectedSince == 0) disconnectedSince = now;
  if (now - disconnectedSince > kPortalAfterMs) {
    Serial.println("Seit 5 Minuten kein WLAN -> Hotspot zur Einrichtung.");
    startSetupPortal();
    return;
  }
  if (now - lastReconnect > kReconnectMs) {
    lastReconnect = now;
    Serial.println("WLAN getrennt - versuche Reconnect...");
    WiFi.disconnect();
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }
}
