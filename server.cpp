#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Server.h"

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