#include "html.h"
#include "config.h"
#include "api.h"

// Webserver-Instanz erstellen
ESP8266WebServer server(80);

void handleRoot() {
    String html = getHtmlPage(selectedCity, prayerAthanModes[0]); // Parameter übergeben
    server.send(200, "text/html", html);
}

void handleSetCity() {
    if (server.hasArg("city")) {
        selectedCity = server.arg("city");
        apiUrl = "http://api.aladhan.com/v1/timingsByCity/" + String(currentDay) + "-" + String(currentMonth) + "-" + String(currentYear) + "?city=" + String(selectedCity) + "&country=Germany&method=2";
        Serial.println("API URL erstellt in handlesetcity funktion: ");
        Serial.println(apiUrl);
        Serial.println("Zeiten abrufen...");
        fetchPrayerTimes(fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, apiUrl);
        server.send(200, "text/plain", "City updated to " + selectedCity);
    } else {
        server.send(400, "text/plain", "Missing 'city' parameter");
    }
}

void handleSetAthan() {
    if (server.hasArg("athan")) {
        int athanMode = server.arg("athan").toInt();
        for (int i = 0; i < 6; i++) {
            prayerAthanModes[i] = athanMode;
        }
        server.send(200, "text/plain", "Athan mode updated to " + String(athanMode));
    } else {
        server.send(400, "text/plain", "Missing 'athan' parameter");
    }
}

String getHtmlPage(String city, int athanMode) {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Athan Clock Einstellungen</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        input, button { margin: 10px; padding: 10px; font-size: 16px; }
        label { font-size: 18px; }
    </style>
</head>
<body>
    <h2>Athan Clock Einstellungen</h2>
    
    <label>Stadt:</label>
    <input type="text" id="city" value=")rawliteral" + city + R"rawliteral(">
    <button onclick="setCity()">Speichern</button>
    <br>
    
    <label>Athan-Modus:</label>
    <select id="athan">
        <option value="0")rawliteral" + (athanMode == 0 ? " selected" : "") + R"rawliteral(>Kein Athan</option>
        <option value="1")rawliteral" + (athanMode == 1 ? " selected" : "") + R"rawliteral(>Athan aktiv</option>
    </select>
    <button onclick="setAthan()">Speichern</button>
    
    <script>
        function setCity() {
            let city = document.getElementById("city").value;
            fetch("/setCity", { 
                method: "POST", 
                headers: { "Content-Type": "application/x-www-form-urlencoded" }, 
                body: "city=" + encodeURIComponent(city) 
            })
            .then(response => response.text())
            .then(data => alert(data));
        }

        function setAthan() {
            let athan = document.getElementById("athan").value;
            fetch("/setAthan", { 
                method: "POST", 
                headers: { "Content-Type": "application/x-www-form-urlencoded" }, 
                body: "athan=" + athan 
            })
            .then(response => response.text())
            .then(data => alert(data));
        }
    </script>
</body>
</html>
)rawliteral";
}