#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_ST7735.h>

enum AppMode { MODE_HOME, MODE_MENU, MODE_EDIT };

extern AppMode currentMode;
extern int menuItem;
extern int menuScroll;

void toggleMenu();
void showMenu(Adafruit_ST7735& display);
void scrollMenu();
void confirmSelection();
void onShortPressUp();
void onShortPressDown();
void volumeUp();
void volumeDown();
void saveAndExit();

#endif
