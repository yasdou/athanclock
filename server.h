#ifndef SERVER_H
#define SERVER_H

#include <ESP8266WebServer.h> // Für ESP8266. Für ESP32: <WebServer.h>

extern ESP8266WebServer server;

void setupServerRoutes();

#endif // SERVER_H
