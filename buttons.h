#pragma once
#include <Arduino.h>

#define BTN_VOL_UP D6
// Achtung: D0 (GPIO16) hat KEINEN internen Pull-up. INPUT_PULLUP wirkt dort nicht,
// der Taster braucht einen externen Pull-up (z. B. 10 kOhm nach 3,3 V).
#define BTN_VOL_DN D0

void setupButtons();
void handleButtons();
bool isAnyButtonPressed();
bool isUpButtonPressed();  // z. B. beim Einschalten gedrückt -> WLAN-Einrichtung erzwingen
