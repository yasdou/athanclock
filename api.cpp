#include "api.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

static String cleanTimeString(const String& raw) {
    if (raw.length() >= 5) return raw.substring(0, 5);
    return raw;
}

bool fetchMonthlyPrayerTimes(PrayerDay monthlyPrayerTimes[], int& daysInMonth, const String& apiUrl) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi ist nicht verbunden!");
        return false;
    }

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();

    HTTPClient http;
    http.useHTTP10(true);  // wichtig gegen chunked transfer beim Stream-Parsing

    Serial.println("======================================");
    Serial.println("Starte Monats-API-Abfrage...");
    Serial.print("API-URL: ");
    Serial.println(apiUrl);

    if (!http.begin(*client, apiUrl)) {
        Serial.println("http.begin() fehlgeschlagen");
        return false;
    }

    int httpCode = http.GET();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);

    if (httpCode <= 0) {
        Serial.print("HTTP Fehlertext: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        return false;
    }

    if (httpCode != HTTP_CODE_OK) {
        Serial.println("API lieferte keinen 200-Status.");
        http.end();
        return false;
    }

    int len = http.getSize();
    Serial.print("Content-Length: ");
    Serial.println(len);

    DynamicJsonDocument filter(1024);
    filter["data"][0]["timings"]["Fajr"] = true;
    filter["data"][0]["timings"]["Sunrise"] = true;
    filter["data"][0]["timings"]["Dhuhr"] = true;
    filter["data"][0]["timings"]["Asr"] = true;
    filter["data"][0]["timings"]["Maghrib"] = true;
    filter["data"][0]["timings"]["Isha"] = true;

    DynamicJsonDocument doc(24576);

    DeserializationError error = deserializeJson(
        doc,
        http.getStream(),
        DeserializationOption::Filter(filter)
    );

    if (error) {
        Serial.print("JSON Parse Fehler: ");
        Serial.println(error.f_str());
        http.end();
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    daysInMonth = data.size();

    Serial.print("Anzahl Tage im Monatsarray: ");
    Serial.println(daysInMonth);

    if (daysInMonth <= 0) {
        Serial.println("Keine Tage gefunden.");
        http.end();
        return false;
    }

    if (daysInMonth > 31) {
        daysInMonth = 31;
    }

    for (int i = 0; i < daysInMonth; i++) {
        JsonObject timings = data[i]["timings"];

        monthlyPrayerTimes[i].fajr    = cleanTimeString(timings["Fajr"]    | "");
        monthlyPrayerTimes[i].shuruk  = cleanTimeString(timings["Sunrise"] | "");
        monthlyPrayerTimes[i].dhuhr   = cleanTimeString(timings["Dhuhr"]   | "");
        monthlyPrayerTimes[i].asr     = cleanTimeString(timings["Asr"]     | "");
        monthlyPrayerTimes[i].maghrib = cleanTimeString(timings["Maghrib"] | "");
        monthlyPrayerTimes[i].isha    = cleanTimeString(timings["Isha"]    | "");

        Serial.print("Tag ");
        Serial.print(i + 1);
        Serial.print(" | Fajr=");
        Serial.print(monthlyPrayerTimes[i].fajr);
        Serial.print(" | Dhuhr=");
        Serial.print(monthlyPrayerTimes[i].dhuhr);
        Serial.print(" | Maghrib=");
        Serial.print(monthlyPrayerTimes[i].maghrib);
        Serial.print(" | Isha=");
        Serial.println(monthlyPrayerTimes[i].isha);
    }

    http.end();
    Serial.println("Monatsdaten erfolgreich eingelesen.");
    Serial.println("======================================");
    return true;
}