#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_ST7735.h>

enum AppMode { MODE_HOME, MODE_MENU, MODE_EDIT };

extern AppMode currentMode;
extern int menuItem;
extern int menuScroll;

bool isMenuOpen();
void toggleMenu();
void showMenu(Adafruit_ST7735& display);
void scrollMenuUp();
void scrollMenuDown();
void confirmSelection();
void saveAndExit();
void showEditScreen(Adafruit_ST7735& display);
void showVolumeOverlay(Adafruit_ST7735& display, int volume);
void updateVolumeOverlay();

#endif