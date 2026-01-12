#ifndef HTML_H
#define HTML_H

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

extern ESP8266WebServer server;  // Deklaration (ohne Initialisierung)

void handleRoot();
void handleSetCity();
void handleSetAthan();

String getHtmlPage(String city, int athanMode);

#endif // HTML_H