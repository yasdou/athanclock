#include "menu.h"
#include "display.h"
#include "audio.h"
#include "config.h"

AppMode currentMode = MODE_HOME;
int menuItem = 0;
int menuScroll = 0;

String cities[] = {"Mainz", "Berlin", "München", "Hamburg", "Köln"};
int numCities = 5;

void toggleMenu() {
    if (currentMode == MODE_HOME) {
        currentMode = MODE_MENU;
        menuItem = 0;
        showMenu(display);
    } else {
        currentMode = MODE_HOME;
        // Zurück zur normalen Anzeige
    }
}

void onShortPressUp() {
    if (currentMode == MODE_HOME) {
        volumeUp();
    } else if (currentMode == MODE_MENU) {
        confirmSelection();
    } else if (currentMode == MODE_EDIT) {
        saveAndExit();
    }
}

void onShortPressDown() {
    if (currentMode == MODE_HOME) {
        volumeDown();
    } else {
        scrollMenu();
    }
}

void showMenu(Adafruit_ST7735& display) {
    display.fillScreen(ST77XX_WHITE);
    display.setTextSize(1);
    display.setTextColor(ST77XX_BLACK);
    display.setCursor(5, 5);
    display.println("=== MENU ===");
    
    display.setCursor(5, 25);
    display.println(menuItem == 0 ? "> 1. Ort" : "  1. Ort");
    display.setCursor(5, 40);
    display.println(menuItem == 1 ? "> 2. Athan" : "  2. Athan");
    display.setCursor(5, 55);
    display.println(menuItem == 2 ? "> 3. Reminder" : "  3. Reminder");
}

void scrollMenu() {
    menuItem = (menuItem + 1) % 3;
    showMenu(display);
}

void confirmSelection() {
    currentMode = MODE_EDIT;
    menuScroll = 0;
    // Edit-Ansicht anzeigen
}

void volumeUp() {
    if (isAudioInitialized) {
        int vol = myDFPlayer.readVolume() + 2;
        if (vol > 30) vol = 30;
        myDFPlayer.volume(vol);
    }
}

void volumeDown() {
    if (isAudioInitialized) {
        int vol = myDFPlayer.readVolume() - 2;
        if (vol < 0) vol = 0;
        myDFPlayer.volume(vol);
    }
}
