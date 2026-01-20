#include "api.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// ===== FORWARD DECLARATION =====
void formatTime(String& timeStr);
void extractPrayerTimes(const String& payload, String& fajr, String& shuruk, String& dhuhr, String& asr, String& maghrib, String& isha);

void fetchPrayerTimes(String& fajr, String& shuruk, String& dhuhr, String& asr, String& maghrib, String& isha, const String& apiUrl) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        // Use WiFiClient class to create TCP connections
        WiFiClientSecure client;
        const int httpPort = 443; // 80 is for HTTP / 443 is for HTTPS!
        
        String url = "https://mawaqit.thesimpleteam.net/times/ikv-kostheim";
                
        client.setInsecure(); // this is the magical line that makes everything work

        Serial.println("\n=== IKV KOSTHEIM ===");
        Serial.println("Start URL: " + url);
        
        http.begin(client, url);
        http.addHeader("User-Agent", "Mozilla/5.0");
        http.addHeader("Accept", "application/json");
        
        int httpCode = http.GET();
        Serial.print("HTTP Code: ");
        Serial.println(httpCode);

       

        if (httpCode == 200) {
            String payload = http.getString();
            Serial.println("✅ Payload: " + payload);
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error && doc.is<JsonArray>()) {
                JsonArray times = doc.as<JsonArray>();
                if (times.size() >= 5) {
                    fajr = times[0].as<String>();
                    dhuhr = times[1].as<String>();
                    asr = times[2].as<String>();
                    maghrib = times[3].as<String>();
                    isha = times[4].as<String>();
                    
                    formatTime(fajr);
                    formatTime(dhuhr);
                    formatTime(asr);
                    formatTime(maghrib);
                    formatTime(isha);
                    
                    Serial.println("\n✅ GEBETSZEITEN:");
                    Serial.print("Fajr:   "); Serial.println(fajr);
                    Serial.print("Dhuhr:  "); Serial.println(dhuhr);
                    Serial.print("Asr:    "); Serial.println(asr);
                    Serial.print("Maghrib:"); Serial.println(maghrib);
                    Serial.print("Isha:   "); Serial.println(isha);
                }
            }
        } else {
            Serial.print("❌ Final Error: ");
            Serial.println(httpCode);
        }
        
        http.end();
    }
}

void formatTime(String& timeStr) {
    int colonPos = timeStr.indexOf(':');
    if (colonPos > 0) {
        timeStr = timeStr.substring(colonPos - 2, colonPos + 3);
    }
}

// ===== EXTRAKTION =====
void extractPrayerTimes(const String& payload, String& fajr, String& shuruk, String& dhuhr, String& asr, String& maghrib, String& isha) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("✗ JSON Fehler: ");
        Serial.println(error.f_str());
        return;
    }

    if (doc.is<JsonArray>()) {
        JsonArray times = doc.as<JsonArray>();
        
        if (times.size() >= 5) {
            fajr = times[0].as<String>();
            dhuhr = times[1].as<String>();
            asr = times[2].as<String>();
            maghrib = times[3].as<String>();
            isha = times[4].as<String>();
            
            // Format zu HH:MM
            formatTime(fajr);
            formatTime(dhuhr);
            formatTime(asr);
            formatTime(maghrib);
            formatTime(isha);
            
            Serial.println("✓ Alle 5 Zeiten geparst & formatiert!");
        } else {
            Serial.println("✗ Zu wenige Zeiten im Array");
        }
    } else {
        Serial.println("✗ Kein JSON-Array");
    }
}

