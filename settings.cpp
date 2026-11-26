#include "settings.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "audio.h"
#include "config.h"
#include "prayer_logic.h"

static const char* kSettingsFile = "/settings.json";
static const char* kSettingsTmp = "/settings.tmp";
static const unsigned long kSaveDelayMs = 2000;  // mehrere Änderungen bündeln (schont den Flash)
static const int kToneCount = 6;

static bool fsReady = false;
static bool dirty = false;
static unsigned long dirtySince = 0;

void markSettingsDirty() {
  dirty = true;
  dirtySince = millis();
}

static int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void loadSettings() {
  fsReady = LittleFS.begin();  // formatiert bei Bedarf automatisch
  if (!fsReady) {
    Serial.println("LittleFS nicht verfuegbar - Einstellungen werden nicht gespeichert.");
    return;
  }

  File f = LittleFS.open(kSettingsFile, "r");
  if (!f) {
    Serial.println("Keine gespeicherten Einstellungen, nutze Standardwerte.");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.print("Einstellungen beschaedigt, nutze Standardwerte: ");
    Serial.println(err.c_str());
    return;
  }

  const char* city = doc["city"] | "";
  if (strlen(city) > 0 && strlen(city) <= 60) {
    currentLocation.name = city;
    currentLocation.region = doc["region"] | "";
    // Ältere Einstellungen haben nur den Namen -> Suche über den Namen wie bisher.
    double lat = doc["lat"] | 1000.0;
    double lon = doc["lon"] | 1000.0;
    currentLocation.hasCoords = prayer::isValidCoordinate(lat, lon);
    currentLocation.latitude = currentLocation.hasCoords ? lat : 0;
    currentLocation.longitude = currentLocation.hasCoords ? lon : 0;
  }

  int method = doc["method"] | calcMethod;
  if (prayer::isValidMethod(method)) calcMethod = method;
  asrSchool = clampInt(doc["school"] | asrSchool, 0, 1);

  JsonArrayConst offsets = doc["offsets"];
  if (offsets.size() == 6) {
    for (int i = 0; i < 6; i++) {
      prayerOffsets[i] =
          clampInt(offsets[i] | 0, -prayer::kMaxOffsetMinutes, prayer::kMaxOffsetMinutes);
    }
  }

  athanTone = String(clampInt(doc["athanTone"] | athanTone.toInt(), 0, kToneCount - 1));
  reminderTone = String(clampInt(doc["reminderTone"] | reminderTone.toInt(), 0, kToneCount - 1));
  fajrTone = String(clampInt(doc["fajrTone"] | fajrTone.toInt(), 0, kToneCount - 1));
  fajrVolume = clampInt(doc["fajrVolume"] | fajrVolume, 0, 30);

  JsonArrayConst athanModes = doc["athanModes"];
  if (athanModes.size() == 6) {
    for (int i = 0; i < 6; i++) prayerAthanModes[i] = clampInt(athanModes[i] | prayerAthanModes[i], 0, 1);
  }
  JsonArrayConst reminderModes = doc["reminderModes"];
  if (reminderModes.size() == 6) {
    for (int i = 0; i < 6; i++) prayerReminderModes[i] = clampInt(reminderModes[i] | prayerReminderModes[i], 0, 2);
  }

  setVolume(doc["volume"] | getCurrentVolume());

  dirty = false;  // setVolume() hat dirty gesetzt, es wurde aber nur geladen
  Serial.print("Einstellungen geladen. Ort: ");
  Serial.println(currentLocation.name);
}

static bool writeSettings() {
  JsonDocument doc;
  doc["city"] = currentLocation.name;
  doc["region"] = currentLocation.region;
  if (currentLocation.hasCoords) {
    doc["lat"] = currentLocation.latitude;
    doc["lon"] = currentLocation.longitude;
  }
  doc["method"] = calcMethod;
  doc["school"] = asrSchool;
  JsonArray offsets = doc["offsets"].to<JsonArray>();
  for (int i = 0; i < 6; i++) offsets.add(prayerOffsets[i]);
  doc["athanTone"] = athanTone.toInt();
  doc["reminderTone"] = reminderTone.toInt();
  doc["fajrTone"] = fajrTone.toInt();
  doc["fajrVolume"] = fajrVolume;
  doc["volume"] = getCurrentVolume();
  JsonArray athanModes = doc["athanModes"].to<JsonArray>();
  JsonArray reminderModes = doc["reminderModes"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    athanModes.add(prayerAthanModes[i]);
    reminderModes.add(prayerReminderModes[i]);
  }

  // Erst in eine Temp-Datei schreiben, dann umbenennen: bei Stromausfall
  // bleibt die alte Datei intakt.
  File f = LittleFS.open(kSettingsTmp, "w");
  if (!f) return false;
  size_t written = serializeJson(doc, f);
  f.close();
  if (written == 0) return false;

  LittleFS.remove(kSettingsFile);
  return LittleFS.rename(kSettingsTmp, kSettingsFile);
}

void saveSettingsIfDirty() {
  if (!dirty || !fsReady) return;
  if (millis() - dirtySince < kSaveDelayMs) return;
  dirty = false;
  if (writeSettings()) {
    Serial.println("Einstellungen gespeichert.");
  } else {
    Serial.println("Einstellungen konnten nicht gespeichert werden.");
  }
}
