#include <ArduinoJson.h>
#include "config.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


// HTTP-Server auf Port 80
ESP8266WebServer server(80);

// Starte HTTP-Server
void startServer() {
  // Definiere Routen
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "AthanClock ist erreichbar!");
  });

  server.on("/settings", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      Serial.println("Einstellungen empfangen: " + body);
      // Verarbeite die Einstellungen hier
      server.send(200, "text/plain", "Einstellungen aktualisiert");
    } else {
      server.send(400, "text/plain", "Keine Daten empfangen");
    }
  });

  server.begin();
  Serial.println("HTTP-Server gestartet!");
}

// Setze mDNS
void setupMDNS() {
  if (MDNS.begin("athanclock")) { // Hostname: athanclock.local
    Serial.println("mDNS gestartet: athanclock.local");
  } else {
    Serial.println("Fehler beim Starten des mDNS!");
  }
}

// Verarbeite eingehende Anfragen
void handleClientRequests() {
  server.handleClient();
}

void handleSaveSettings() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<1024> json;
    DeserializationError error = deserializeJson(json, body);

    if (error) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    // Einstellungen für jede Gebetszeit extrahieren
    int fajrReminder = json["prayers"]["Fajr"]["reminder"];
    int fajrAthan = json["prayers"]["Fajr"]["athan"];

    int shurukReminder = json["prayers"]["Shuruk"]["reminder"];
    int shurukAthan = json["prayers"]["Shuruk"]["athan"];

    int dhuhrReminder = json["prayers"]["Dhuhr"]["reminder"];
    int dhuhrAthan = json["prayers"]["Dhuhr"]["athan"];

    int asrReminder = json["prayers"]["Asr"]["reminder"];
    int asrAthan = json["prayers"]["Asr"]["athan"];

    int maghribReminder = json["prayers"]["Maghrib"]["reminder"];
    int maghribAthan = json["prayers"]["Maghrib"]["athan"];

    int ishaReminder = json["prayers"]["Isha"]["reminder"];
    int ishaAthan = json["prayers"]["Isha"]["athan"];

    // Stadt und Töne extrahieren
    selectedCity = json["city"].as<String>();
    athanTone = json["athan"].as<String>();
    reminderTone = json["reminderTone"].as<String>();

    // Hier kannst du die empfangenen Einstellungen verarbeiten
    Serial.println("Einstellungen empfangen:");
    Serial.printf("Fajr Reminder: %d, Fajr Athan: %d\n", fajrReminder, fajrAthan);
    Serial.printf("Shuruk Reminder: %d, Shuruk Athan: %d\n", shurukReminder, shurukAthan);
    Serial.printf("Dhuhr Reminder: %d, Dhuhr Athan: %d\n", dhuhrReminder, dhuhrAthan);
    Serial.printf("Asr Reminder: %d, Asr Athan: %d\n", asrReminder, asrAthan);
    Serial.printf("Maghrib Reminder: %d, Maghrib Athan: %d\n", maghribReminder, maghribAthan);
    Serial.printf("Isha Reminder: %d, Isha Athan: %d\n", ishaReminder, ishaAthan);

    Serial.printf("Stadt: %s\n", selectedCity.c_str());
    Serial.printf("Athan: %s, Reminder: %s\n", athanTone.c_str(), reminderTone.c_str());

    // Antwort senden
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
  }
}