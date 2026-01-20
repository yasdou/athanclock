#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

#define BTN_VOL_UP D6
#define BTN_VOL_DN D0

void setupButtons();
void handleButtons();
bool isAnyButtonPressed();

#endif
