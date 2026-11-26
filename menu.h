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
void scrollMenuUp();
void scrollMenuDown();
void confirmSelection();
void saveAndExit();
bool isMenuOpen();
void goBackOneLevel();
void showVolumeOverlay(Adafruit_ST7735& display, int volume);
void showEditScreen(Adafruit_ST7735& display);
void saveSelection();
void updateVolumeOverlay();
void updateMenuTimeout();

#endif
