#include "api.h"
#include "config.h"
#include <ESP8266WiFi.h>

#include <ArduinoJson.h> // Wenn noch nicht inkludiert, für JSON-Dokumente
// #include <WiFi.h>  // Stelle sicher, dass du WiFi.h inkludiert hast

void fetchPrayerTimes(String& fajr, String& shuruk, String& dhuhr, String& asr, String& maghrib, String& isha, const String& apiUrl) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;  // HTTPClient-Objekt deklarieren
        WiFiClient client;
        http.begin(client, apiUrl); // Benutze WiFiClient-Objekt
        Serial.println("Starte API-Abfrage...");
        Serial.println("API-URL:");
        Serial.println(apiUrl);

        int httpCode = http.GET(); // Sende GET-Anfrage        
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);  // Zeigt den Statuscode an

        if (httpCode == 200) {
            // Wenn die Anfrage erfolgreich war
            String payload = http.getString();
            Serial.println("API-Abfrage erfolgreich!");
            Serial.println("Raw API response:");
            Serial.println(payload);  // Zeigt die rohe Antwort an

            // JSON-Daten deserialisieren
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                Serial.print("Fehler beim Parsen des JSON: ");
                Serial.println(error.f_str());
                return;
            }

            // Gebetszeiten extrahieren
            fajr = doc["data"]["timings"]["Fajr"].as<String>();
            shuruk = doc["data"]["timings"]["Sunrise"].as<String>();
            dhuhr = doc["data"]["timings"]["Dhuhr"].as<String>();
            asr = doc["data"]["timings"]["Asr"].as<String>();
            maghrib = doc["data"]["timings"]["Maghrib"].as<String>();
            isha = doc["data"]["timings"]["Isha"].as<String>();

            // Ausgeben der abgerufenen Gebetszeiten im Serial Monitor
            Serial.println("Abgerufene Gebetszeiten:");
            Serial.print("Fajr: "); Serial.println(fajr);
            Serial.print("Shuruk: "); Serial.println(shuruk);
            Serial.print("Dhuhr: "); Serial.println(dhuhr);
            Serial.print("Asr: "); Serial.println(asr);
            Serial.print("Maghrib: "); Serial.println(maghrib);
            Serial.print("Isha: "); Serial.println(isha);
        // } else if (httpCode == 302) {
        //     // Umleitung (Redirect)
        //     Serial.println("Fehler 302: Umleitung");
        //     String redirectUrl = http.header("Location");
        //     if (redirectUrl != "") {
        //         Serial.print("Weiterleitungs-URL: ");
        //         Serial.println(redirectUrl);
        //         httpCode = http.GET();  // Sende die GET-Anfrage erneut an die neue URL
        //         if (httpCode == 200) {
        //             String payload = http.getString();
        //             Serial.println("API-Abfrage erfolgreich nach Umleitung!");
        //             Serial.println(payload);  // Zeigt die Antwort nach Umleitung an
        //         } else {
        //             Serial.print("Fehler bei der API-Abfrage nach Umleitung. HTTP Code: ");
        //             Serial.println(httpCode);
        //         }
            // } else {
            //     Serial.println("Keine Weiterleitungs-URL gefunden.");
            //     // Zeige alle Header, um mehr Informationen zu erhalten
            //     Serial.println("Antwort-Header:");
            //     String headers = http.getString();
            //     Serial.println(headers);
            // }
        } else {
            // Fehler bei der HTTP-Anfrage
            Serial.print("Fehler bei der API-Abfrage. HTTP Code: ");
            Serial.println(httpCode);
        }

        http.end(); // HTTP-Verbindung beenden
    } else {
        // Kein Wi-Fi verbunden
        Serial.println("WiFi ist nicht verbunden!");
    }
}
