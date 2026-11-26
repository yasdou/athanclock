#include "html.h"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "audio.h"
#include "config.h"
#include "display.h"
#include "prayer_logic.h"
#include "web_page.h"
#include "wifi_page.h"
#include "wifi_setup.h"

ESP8266WebServer server(80);
static ESP8266HTTPUpdateServer httpUpdater;
static bool otaStarted = false;
static bool otaFromArduino = false;

static const int kToneCount = 6;
static const int kMaxVolume = 30;
static const size_t kMaxBodySize = 1024;

static bool mdnsStarted = false;

// ---------------------------------------------------------------------------
// Hilfsfunktionen

static void sendJson(int code, const JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", out);
}

static void sendError(int code, const char* message) {
  JsonDocument doc;
  doc["error"] = message;
  sendJson(code, doc);
}

static bool parseBody(JsonDocument& doc) {
  if (!server.hasArg("plain")) return false;
  const String& body = server.arg("plain");
  if (body.length() == 0 || body.length() > kMaxBodySize) return false;
  return !deserializeJson(doc, body);
}

// Liest ein optionales Ganzzahl-Feld. false = Feld vorhanden, aber ungültig.
static bool readOptionalInt(JsonVariantConst v, int lo, int hi, int& target) {
  if (v.isNull()) return true;
  if (!v.is<int>()) return false;
  int x = v.as<int>();
  if (x < lo || x > hi) return false;
  target = x;
  return true;
}

// Liest ein optionales Array mit 6 Ganzzahlen im Bereich lo..hi.
static bool readOptionalModes(JsonVariantConst v, int lo, int hi, int target[6]) {
  if (v.isNull()) return true;
  JsonArrayConst arr = v.as<JsonArrayConst>();
  if (arr.isNull() || arr.size() != 6) return false;
  int tmp[6];
  for (int i = 0; i < 6; i++) {
    if (!arr[i].is<int>()) return false;
    tmp[i] = arr[i].as<int>();
    if (tmp[i] < lo || tmp[i] > hi) return false;
  }
  memcpy(target, tmp, sizeof(tmp));
  return true;
}

static void fillState(JsonDocument& doc) {
  struct tm t;
  bool synced = readLocalTime(t);
  doc["synced"] = synced;
  if (synced) {
    char date[40];  // großzügig, damit der Compiler keine Kürzung befürchtet
    snprintf(date, sizeof(date), "%02d.%02d.%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    doc["date"] = date;
    doc["weekday"] = t.tm_wday;
    doc["sec"] = t.tm_hour * 3600L + t.tm_min * 60L + t.tm_sec;
  }

  doc["city"] = currentLocation.name;
  doc["region"] = currentLocation.region;
  doc["hasCoords"] = currentLocation.hasCoords;
  doc["method"] = calcMethod;
  doc["school"] = asrSchool;

  int today[6], upcoming[6];
  getPrayerOverview(today, upcoming);
  JsonArray todayArr = doc["today"].to<JsonArray>();
  JsonArray upcomingArr = doc["upcoming"].to<JsonArray>();
  JsonArray athanArr = doc["athanModes"].to<JsonArray>();
  JsonArray reminderArr = doc["reminderModes"].to<JsonArray>();
  JsonArray offsetArr = doc["offsets"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    todayArr.add(today[i]);
    upcomingArr.add(upcoming[i]);
    athanArr.add(prayer::hasAthan(i) ? prayerAthanModes[i] : 0);
    reminderArr.add(prayerReminderModes[i]);
    offsetArr.add(prayerOffsets[i]);
  }

  doc["athanTone"] = athanTone.toInt();
  doc["reminderTone"] = reminderTone.toInt();
  doc["fajrTone"] = fajrTone.toInt();
  doc["fajrVolume"] = fajrVolume;
  doc["ownFajr"] = hasOwnFajrAthans();
  doc["volume"] = getCurrentVolume();
  doc["playing"] = isAudioPlaying();

  doc["version"] = FIRMWARE_VERSION;
  doc["wifi"] = savedWifiSsid();
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["heap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
}

// ---------------------------------------------------------------------------
// Handler

// Im Hotspot jede Anfrage im seriellen Monitor zeigen (hilft bei Problemen mit dem Captive Portal)
static void logPortalRequest() {
  if (!isSetupMode()) return;
  Serial.printf("[Portal] %s http://%s%s (Clients am Hotspot: %d)\n",
                server.method() == HTTP_GET ? "GET" : "POST", server.hostHeader().c_str(), server.uri().c_str(),
                WiFi.softAPgetStationNum());
}

static void handleRoot() {
  logPortalRequest();
  server.sendHeader("Cache-Control", "no-cache");
  // Im Einrichtungsmodus gibt es nur die WLAN-Seite (kein Internet, keine Zeiten).
  server.send_P(200, "text/html; charset=utf-8", isSetupMode() ? WIFI_PAGE : WEB_PAGE);
}

static void handleWifiPage() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html; charset=utf-8", WIFI_PAGE);
}

static void handleWifiStatus() {
  JsonDocument doc;
  doc["setupMode"] = isSetupMode();
  doc["connected"] = WiFi.status() == WL_CONNECTED;
  doc["ssid"] = savedWifiSsid();
  doc["apName"] = setupApName();
  if (WiFi.status() == WL_CONNECTED) doc["ip"] = WiFi.localIP().toString();
  sendJson(200, doc);
}

// Netzwerksuche im Hintergrund: Eine blockierende Suche schickt den Hotspot ~2 s
// auf andere Kanäle und kann das verbundene Handy trennen. Die Seite fragt daher
// so lange nach, bis "scanning" false ist. ?refresh=1 startet eine neue Suche.
static void handleWifiScan() {
  JsonDocument doc;
  int n = WiFi.scanComplete();  // WIFI_SCAN_RUNNING (-1), WIFI_SCAN_FAILED (-2) oder Anzahl
  if (n == WIFI_SCAN_RUNNING) {
    doc["scanning"] = true;
    sendJson(200, doc);
    return;
  }
  if (n < 0 || server.hasArg("refresh")) {
    WiFi.scanNetworks(true);
    doc["scanning"] = true;
    sendJson(200, doc);
    return;
  }

  doc["scanning"] = false;
  JsonArray arr = doc["networks"].to<JsonArray>();
  // Nach Signalstärke sortiert, doppelte Namen (Mesh/Repeater) nur einmal
  bool used[64] = {false};
  int count = n < 64 ? n : 64;
  for (int k = 0; k < count && arr.size() < 15; k++) {
    int best = -1;
    for (int i = 0; i < count; i++) {
      if (used[i]) continue;
      if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
    }
    if (best < 0) break;
    used[best] = true;
    String name = WiFi.SSID(best);
    if (name.length() == 0) continue;  // versteckte Netze
    bool duplicate = false;
    for (JsonObject o : arr) {
      if (name == o["ssid"].as<const char*>()) duplicate = true;
    }
    if (duplicate) continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = name;
    o["rssi"] = WiFi.RSSI(best);
    o["secure"] = WiFi.encryptionType(best) != ENC_TYPE_NONE;
  }
  // Ergebnisse nicht löschen: weitere Aufrufe bekommen dieselbe Liste ohne neue Suche
  sendJson(200, doc);
}

static void handleWifiSave() {
  // Im Normalbetrieb nur mit Geräte-Passwort (sonst könnte jeder im Netz das WLAN verstellen).
  // Im Einrichtungsmodus ist man ohnehin nur mit diesem Passwort im Hotspot.
  if (!isSetupMode() && server.header("X-Device-Password") != devicePassword()) {
    sendError(403, "Geräte-Passwort falsch. Es steht im Menü der Uhr unter „Info“.");
    return;
  }

  JsonDocument body;
  if (!parseBody(body)) {
    sendError(400, "Ungültige Anfrage (JSON erwartet).");
    return;
  }
  String newSsid = body["ssid"] | "";
  String newPass = body["pass"] | "";
  if (newSsid.length() == 0 || newSsid.length() > 32) {
    sendError(400, "Bitte einen WLAN-Namen eingeben (max. 32 Zeichen).");
    return;
  }
  if (newPass.length() > 63 || (newPass.length() > 0 && newPass.length() < 8)) {
    sendError(400, "Das WLAN-Passwort muss 8 bis 63 Zeichen haben (oder leer sein).");
    return;
  }

  JsonDocument doc;
  if (isSetupMode()) {
    String ip;
    if (!trySetupConnect(newSsid, newPass, &ip)) {
      sendError(400, "Verbindung fehlgeschlagen. Bitte WLAN-Name und Passwort prüfen.");
      return;
    }
    doc["ip"] = ip;
  } else {
    if (!saveWifiCredentials(newSsid, newPass)) {
      sendError(500, "Speichern fehlgeschlagen.");
      return;
    }
    scheduleRestart(2000);
  }
  doc["ok"] = true;
  sendJson(200, doc);
}

static void handleState() {
  JsonDocument doc;
  fillState(doc);
  sendJson(200, doc);
}

static void handleSettings() {
  JsonDocument body;
  if (!parseBody(body)) {
    sendError(400, "Ungültige Anfrage (JSON erwartet).");
    return;
  }

  // Erst alles prüfen, dann übernehmen -> keine halb gespeicherten Änderungen.
  int newAthanTone = athanTone.toInt();
  int newReminderTone = reminderTone.toInt();
  int newFajrTone = fajrTone.toInt();
  int newFajrVolume = fajrVolume;
  int newVolume = getCurrentVolume();
  int newAthanModes[6], newReminderModes[6], newOffsets[6];
  memcpy(newAthanModes, prayerAthanModes, sizeof(newAthanModes));
  memcpy(newReminderModes, prayerReminderModes, sizeof(newReminderModes));
  memcpy(newOffsets, prayerOffsets, sizeof(newOffsets));

  if (!readOptionalInt(body["athanTone"], 0, kToneCount - 1, newAthanTone) ||
      !readOptionalInt(body["reminderTone"], 0, kToneCount - 1, newReminderTone) ||
      !readOptionalInt(body["fajrTone"], 0, kToneCount - 1, newFajrTone) ||
      !readOptionalInt(body["fajrVolume"], 0, kMaxVolume, newFajrVolume) ||
      !readOptionalInt(body["volume"], 0, kMaxVolume, newVolume) ||
      !readOptionalModes(body["athanModes"], 0, 1, newAthanModes) ||
      !readOptionalModes(body["reminderModes"], 0, 2, newReminderModes) ||
      !readOptionalModes(body["offsets"], -prayer::kMaxOffsetMinutes, prayer::kMaxOffsetMinutes, newOffsets)) {
    sendError(400, "Ungültiger Wert.");
    return;
  }
  newAthanModes[prayer::SHURUK] = 0;  // Sonnenaufgang: nie Athan

  athanTone = String(newAthanTone);
  reminderTone = String(newReminderTone);
  fajrTone = String(newFajrTone);
  fajrVolume = newFajrVolume;
  memcpy(prayerAthanModes, newAthanModes, sizeof(newAthanModes));
  memcpy(prayerReminderModes, newReminderModes, sizeof(newReminderModes));
  bool offsetsChanged = memcmp(prayerOffsets, newOffsets, sizeof(newOffsets)) != 0;
  memcpy(prayerOffsets, newOffsets, sizeof(newOffsets));
  if (offsetsChanged) requestScreenRedraw();
  setVolume(newVolume);
  markSettingsDirty();

  handleState();
}

static void handleLocation() {
  JsonDocument body;
  if (!parseBody(body)) {
    sendError(400, "Ungültige Anfrage (JSON erwartet).");
    return;
  }

  // Ohne "name": Ort bleibt, nur die Methode ändert sich.
  // Mit "name": neuer Ort, dann sind Koordinaten Pflicht (aus der Ortssuche der Webseite).
  // Freitext ohne Koordinaten wird abgelehnt, damit kein falscher Ort durchrutscht.
  Location location = currentLocation;
  if (!body["name"].isNull()) {
    if (!body["name"].is<const char*>() || !body["lat"].is<double>() || !body["lon"].is<double>()) {
      sendError(400, "Bitte einen Ort aus der Suche auswählen.");
      return;
    }
    location.name = body["name"].as<const char*>();
    location.region = body["region"] | "";
    location.latitude = body["lat"].as<double>();
    location.longitude = body["lon"].as<double>();
    location.hasCoords = true;
  }

  int method = calcMethod;
  if (!readOptionalInt(body["method"], 0, 23, method) || !prayer::isValidMethod(method)) {
    sendError(400, "Ungültige Berechnungsmethode.");
    return;
  }
  int school = asrSchool;
  if (!readOptionalInt(body["school"], 0, 1, school)) {
    sendError(400, "Ungültige Asr-Berechnung.");
    return;
  }

  // changeLocation() prüft Koordinaten und Zeitzone und übernimmt nur bei Erfolg.
  String error;
  if (!changeLocation(location, method, school, &error)) {
    sendError(502, error.c_str());
    return;
  }
  handleState();
}

static void handlePreview() {
  JsonDocument body;
  if (!parseBody(body)) {
    sendError(400, "Ungültige Anfrage (JSON erwartet).");
    return;
  }
  const char* kind = body["kind"] | "";
  int tone = -1;
  if (!readOptionalInt(body["tone"], 0, kToneCount - 1, tone) || tone < 0) {
    sendError(400, "Ungültiger Ton.");
    return;
  }

  stopAudio();
  if (strcmp(kind, "athan") == 0) {
    playAthan(String(tone));
  } else if (strcmp(kind, "fajr") == 0) {
    playFajrAthan(String(tone), fajrVolume);
  } else if (strcmp(kind, "reminder") == 0) {
    playReminder(String(tone));
  } else {
    sendError(400, "Unbekannte Art (athan/fajr/reminder).");
    return;
  }
  handleState();
}

static void handleStop() {
  stopAudio();
  handleState();
}

static void handleNotFound() {
  logPortalRequest();
  if (server.uri().startsWith("/api/")) {
    sendError(404, "Unbekannter Pfad.");
    return;
  }
  // Alles andere auf die Startseite umleiten. Im Einrichtungsmodus mit voller Adresse:
  // Handys prüfen beim Verbinden z. B. /generate_204 oder /hotspot-detect.html und
  // öffnen bei einer Umleitung automatisch die Einrichtungsseite (Captive Portal).
  server.sendHeader("Location", isSetupMode() ? "http://192.168.4.1/" : "/");
  server.send(302, "text/plain", "");
}

// ---------------------------------------------------------------------------
// Firmware-Updates

static void setupUpdates() {
  // Fortschritt auf dem Display. Beim Web-Upload kennt der Updater die Dateigröße
  // nicht, daher wird gegen die aktuelle Firmware-Größe geschätzt.
  Update.onProgress([](size_t done, size_t total) {
    if (otaFromArduino) return;  // ArduinoOTA meldet selbst
    size_t reference = ESP.getSketchSize();
    int percent = reference ? (int)(done * 100 / reference) : 0;
    if (percent > 99) percent = 99;
    (void)total;
    showUpdateProgress(display, percent);
  });

  // Upload über den Browser: http://athanclock.local/update (Benutzer "admin", Geräte-Passwort)
  httpUpdater.setup(&server, "/update", "admin", devicePassword());
}

static void startArduinoOta() {
  ArduinoOTA.setHostname(WEB_HOSTNAME);
  ArduinoOTA.setPassword(devicePassword());
  ArduinoOTA.onStart([]() {
    otaFromArduino = true;
    stopAudio();
    Serial.println("OTA-Update startet...");
    showUpdateProgress(display, 0);
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    showUpdateProgress(display, total ? (int)(done * 100UL / total) : 0);
  });
  ArduinoOTA.onEnd([]() {
    showUpdateProgress(display, 100);
    Serial.println("OTA-Update fertig, Neustart.");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA-Fehler %u\n", (unsigned)error);
    otaFromArduino = false;
    showUpdateProgress(display, -1);
    requestScreenRedraw();
  });
  ArduinoOTA.begin(false);  // mDNS starten wir selbst
  MDNS.enableArduino(8266, true);
  otaStarted = true;
  Serial.println("ArduinoOTA bereit (Port 8266).");
}

// ---------------------------------------------------------------------------

void setupWebServer() {
  setupUpdates();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on("/api/location", HTTP_POST, handleLocation);
  server.on("/api/preview", HTTP_POST, handlePreview);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/wifi", HTTP_GET, handleWifiPage);
  server.on("/api/wifi/status", HTTP_GET, handleWifiStatus);
  server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);
  server.onNotFound(handleNotFound);
  server.collectHeaders("X-Device-Password");
  server.begin();
  Serial.println("Webserver gestartet: http://" WEB_HOSTNAME ".local");
}

void updateWebServer() {
  server.handleClient();

  // mDNS erst starten, wenn WLAN steht (auch wenn das erst nach dem Start klappt)
  if (!mdnsStarted && !isSetupMode() && WiFi.status() == WL_CONNECTED) {
    mdnsStarted = MDNS.begin(WEB_HOSTNAME);
    if (mdnsStarted) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("mDNS aktiv: http://" WEB_HOSTNAME ".local");
      startArduinoOta();
    }
  }
  if (mdnsStarted) MDNS.update();
  if (otaStarted) ArduinoOTA.handle();
}
