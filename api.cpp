#include "api.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>

#include "month_parser.h"

static uint32_t minFreeHeap = 0;

static void recordMinHeap() {
  uint32_t heap = ESP.getFreeHeap();
  if (heap < minFreeHeap) minFreeHeap = heap;
}

// Eigentlicher Abruf. Eigene Funktion, damit TLS-Client und HTTPClient beim
// Verlassen freigegeben sind, bevor der Speicher danach gemessen wird.
static bool fetchWithClient(const char* apiUrl, int year, int month, prayer::MonthTimes& out) {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();  // kein Zertifikats-Check (spart RAM, Daten sind nicht geheim)
  // aladhan unterstützt kein MFLN -> der 16-KB-Empfangspuffer ist nötig.

  HTTPClient http;
  http.useHTTP10(true);  // kein Chunked-Encoding -> Stream direkt parsbar
  http.setTimeout(15000);

  if (!http.begin(*client, apiUrl)) {
    Serial.println("http.begin() fehlgeschlagen");
    return false;
  }

  int httpCode = http.GET();
  recordMinHeap();
  Serial.printf("HTTP Status Code: %d\n", httpCode);
  if (httpCode != HTTP_CODE_OK) {
    if (httpCode <= 0) {
      Serial.print("HTTP Fehlertext: ");
      Serial.println(http.errorToString(httpCode));
    }
    http.end();
    return false;
  }

  const char* error = "";
  bool ok = prayer::parseMonthStream(http.getStream(), year, month, out, &error, recordMinHeap);
  http.end();

  if (!ok) {
    Serial.print("Monatsdaten ungueltig: ");
    Serial.println(error);
  }
  return ok;
}

bool fetchMonthlyPrayerTimes(const char* apiUrl, int year, int month, prayer::MonthTimes& out) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi ist nicht verbunden!");
    return false;
  }

  Serial.println("======================================");
  Serial.print("API-URL: ");
  Serial.println(apiUrl);
  Serial.printf("Heap vor Abruf: %u frei, groesster Block %u\n", ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());

  minFreeHeap = ESP.getFreeHeap();
  bool ok = fetchWithClient(apiUrl, year, month, out);

  // Groesster Block < ~20 KB -> der naechste TLS-Abruf kann scheitern (Fragmentierung).
  Serial.printf("Heap: Minimum waehrend Abruf %u, danach %u frei, groesster Block %u\n", minFreeHeap,
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());

  if (ok) Serial.printf("Monat %02d/%d geladen (%d Tage).\n", month, year, out.days);
  Serial.println("======================================");
  return ok;
}
