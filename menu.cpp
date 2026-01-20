#include "menu.h"
#include "display.h"
#include "audio.h"
#include "config.h"

extern Adafruit_ST7735 display;
extern DFRobotDFPlayerMini myDFPlayer;
extern bool isAudioInitialized;
extern String selectedCity;
extern String athanTone;
extern String reminderTone;

AppMode currentMode = MODE_HOME;
int menuItem = 0;
int menuScroll = 0;

String cities[] = {"Mainz", "Berlin", "München", "Hamburg", "Köln"};
int numCities = 5;

void toggleMenu() {
    Serial.println("toggleMenu() aufgerufen!");
    Serial.print("Aktueller Modus: ");
    Serial.println(currentMode == MODE_HOME ? "HOME" : "MENU");
    
    if (currentMode == MODE_HOME) {
        currentMode = MODE_MENU;
        menuItem = 0;
        showMenu(display);
        Serial.println("→ MODE_MENU");
    } else {
        currentMode = MODE_HOME;
        Serial.println("→ MODE_HOME");
        // Normale Anzeige
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
    if (!isAudioInitialized) {
        Serial.println("Audio nicht verfügbar");
        return;
    }
    
    int vol = myDFPlayer.readVolume();
    if (vol < 30) {
        vol += 2;
        myDFPlayer.volume(vol);
        Serial.print("Lautstärke: ");
        Serial.println(vol);
        
        // Feedback auf Display (optional)
        display.fillRect(0, 120, 128, 20, ST77XX_WHITE);
        display.setCursor(10, 125);
        display.setTextColor(ST77XX_BLACK);
        display.setTextSize(1);
        display.print("Vol: ");
        display.print(vol);
    }
}

void volumeDown() {
    if (!isAudioInitialized) {
        Serial.println("Audio nicht verfügbar");
        return;
    }
    
    int vol = myDFPlayer.readVolume();
    if (vol > 0) {
        vol -= 2;
        myDFPlayer.volume(vol);
        Serial.print("Lautstärke: ");
        Serial.println(vol);
        
        // Feedback auf Display (optional)
        display.fillRect(0, 120, 128, 20, ST77XX_WHITE);
        display.setCursor(10, 125);
        display.setTextColor(ST77XX_BLACK);
        display.setTextSize(1);
        display.print("Vol: ");
        display.print(vol);
    }
}

void saveAndExit() {
    // Speichere aktuelle Auswahl
    switch(menuItem) {
        case 0: // Stadt
            selectedCity = cities[menuScroll];
            Serial.print("Stadt gespeichert: ");
            Serial.println(selectedCity);
            break;
            
        case 1: // Athan
            athanTone = String(menuScroll);
            Serial.print("Athan gespeichert: ");
            Serial.println(athanTone);
            break;
            
        case 2: // Reminder
            reminderTone = String(menuScroll);
            Serial.print("Reminder gespeichert: ");
            Serial.println(reminderTone);
            break;
    }
    
    // Zurück zum HOME-Modus
    currentMode = MODE_HOME;
    display.fillScreen(ST77XX_WHITE);
    Serial.println("Einstellungen gespeichert → Home");
}
