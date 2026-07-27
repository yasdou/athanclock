#include "html.h"
#include "config.h"
#include "api.h"
#include <time.h>

ESP8266WebServer server(80);

extern String selectedCity;
extern String apiUrl;

extern int currentDay;
extern int currentMonth;
extern int currentYear;
extern int loadedMonth;
extern int loadedYear;
extern int daysInLoadedMonth;

extern PrayerDay monthlyPrayerTimes[31];

extern String fajrTime;
extern String shurukTime;
extern String dhuhrTime;
extern String asrTime;
extern String maghribTime;
extern String ishaTime;

String buildMonthlyApiUrlHtml(int year, int month) {
    return "https://api.aladhan.com/v1/calendarByCity/" + String(year) + "/" + String(month) +
           "?city=" + String(selectedCity) + "&country=Germany&method=2";
}

void refreshTodayPrayerTimesFromCache() {
    int todayIndex = currentDay - 1;

    if (todayIndex >= 0 && todayIndex < daysInLoadedMonth) {
        fajrTime    = monthlyPrayerTimes[todayIndex].fajr;
        shurukTime  = monthlyPrayerTimes[todayIndex].shuruk;
        dhuhrTime   = monthlyPrayerTimes[todayIndex].dhuhr;
        asrTime     = monthlyPrayerTimes[todayIndex].asr;
        maghribTime = monthlyPrayerTimes[todayIndex].maghrib;
        ishaTime    = monthlyPrayerTimes[todayIndex].isha;
    }
}

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Athan Clock</title>
</head>
<body>
  <h1>Athan Clock</h1>

  <form action="/setCity" method="POST">
    <label for="city">Stadt:</label>
    <input type="text" id="city" name="city">
    <button type="submit">Speichern</button>
  </form>

  <form action="/setAthan" method="POST">
    <label for="athan">Athan:</label>
    <input type="text" id="athan" name="athan">
    <button type="submit">Speichern</button>
  </form>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void handleSetCity() {
    if (server.hasArg("city")) {
        selectedCity = server.arg("city");
        apiUrl = buildMonthlyApiUrlHtml(currentYear, currentMonth);

        if (fetchMonthlyPrayerTimes(monthlyPrayerTimes, daysInLoadedMonth, apiUrl)) {
            loadedMonth = currentMonth;
            loadedYear = currentYear;
            refreshTodayPrayerTimesFromCache();
            server.send(200, "text/plain", "Stadt gespeichert und Gebetszeiten aktualisiert: " + selectedCity);
            return;
        }

        server.send(500, "text/plain", "Stadt gespeichert, aber Gebetszeiten konnten nicht geladen werden.");
        return;
    }

    server.send(400, "text/plain", "Keine Stadt empfangen.");
}

void handleSetAthan() {
    if (server.hasArg("athan")) {
        String athan = server.arg("athan");
        server.send(200, "text/plain", "Athan gespeichert: " + athan);
        return;
    }

    server.send(400, "text/plain", "Kein Athan empfangen.");
}