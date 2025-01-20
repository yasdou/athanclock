#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);

void handleSaveSettings() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<1024> json;
    DeserializationError error = deserializeJson(json, body);

    if (error) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    // Einstellungen extrahieren
    bool fajrReminder = json["prayers"]["Fajr"]["reminder"];
    bool fajrAthan = json["prayers"]["Fajr"]["athan"];
    String selectedCity = json["city"];
    String athanTone = json["athan"];
    String reminderTone = json["reminderTone"];

    // Hier kannst du die empfangenen Einstellungen verarbeiten
    Serial.println("Einstellungen empfangen:");
    Serial.printf("Fajr Reminder: %d, Fajr Athan: %d\n", fajrReminder, fajrAthan);
    Serial.printf("Stadt: %s\n", selectedCity.c_str());
    Serial.printf("Athan: %s, Reminder: %s\n", athanTone.c_str(), reminderTone.c_str());

    // Antwort senden
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi verbunden!");

  server.on("/save-settings", HTTP_POST, handleSaveSettings);
  server.begin();
  Serial.println("Server läuft auf Port 80");
}

void loop() {
  server.handleClient();
}