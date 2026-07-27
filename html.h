// html.h
#ifndef HTML_H
#define HTML_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void handleRoot();
void handleSetCity();
void handleSetAthan();

#endif