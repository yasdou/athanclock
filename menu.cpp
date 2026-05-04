#include "menu.h"
#include "display.h"
#include "audio.h"
#include "config.h"

extern Adafruit_ST7735 display;
extern String selectedCity;
extern String athanTone;
extern String reminderTone;

AppMode currentMode = MODE_HOME;
int menuItem = 0;
int menuScroll = 0;

unsigned long volumeOverlayShownAt = 0;
bool volumeOverlayActive = false;
const unsigned long VOLUME_OVERLAY_DURATION_MS = 2000;

String cities[] = {"Mainz", "Berlin", "Muenchen", "Hamburg", "Koeln"};
int numCities = 5;
int numAthanTones = 6;
int numReminderTones = 6;

bool isMenuOpen() {
    return currentMode == MODE_MENU || currentMode == MODE_EDIT;
}

void toggleMenu() {
    if (currentMode == MODE_HOME) {
        currentMode = MODE_MENU;
        menuItem = 0;
        showMenu(display);
        Serial.println("-> MODE_MENU");
    } else {
        currentMode = MODE_HOME;
        display.fillScreen(ST77XX_WHITE);
        Serial.println("-> MODE_HOME");
    }
}

void showMenu(Adafruit_ST7735& display) {
    display.fillScreen(ST77XX_WHITE);
    display.setTextSize(1);

    display.fillRect(0, 0, 128, 16, ST77XX_BLUE);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(30, 4);
    display.println("=  MENU  =");

    const char* items[] = {"1. Stadt", "2. Athan-Ton", "3. Reminder-Ton"};
    for (int i = 0; i < 3; i++) {
        int yPos = 25 + i * 18;
        if (i == menuItem) {
            display.fillRect(0, yPos - 2, 128, 16, ST77XX_CYAN);
        }
        display.setTextColor(ST77XX_BLACK);
        display.setCursor(8, yPos);
        display.println(items[i]);
    }

    display.setTextColor(0x4208);  // Grau
    display.setCursor(2, 110);
    display.println("Kurz=Nav  Lang=Ausw.");
}

void showEditScreen(Adafruit_ST7735& display) {
    display.fillScreen(ST77XX_WHITE);
    display.setTextSize(1);

    display.fillRect(0, 0, 128, 16, ST77XX_BLUE);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(5, 4);

    switch (menuItem) {
        case 0: display.println("  Stadt waehlen");   break;
        case 1: display.println("  Athan-Ton");        break;
        case 2: display.println("  Reminder-Ton");     break;
    }

    display.setTextColor(ST77XX_BLACK);

    int maxItems = 0;
    if (menuItem == 0) maxItems = numCities;
    else if (menuItem == 1) maxItems = numAthanTones;
    else maxItems = numReminderTones;

    int visibleStart = max(0, min(menuScroll, maxItems - 4));
    for (int i = visibleStart; i < min(maxItems, visibleStart + 4); i++) {
        int yPos = 25 + (i - visibleStart) * 18;
        if (i == menuScroll) {
            display.fillRect(0, yPos - 2, 128, 16, ST77XX_CYAN);
        }
        display.setTextColor(ST77XX_BLACK);
        display.setCursor(8, yPos);
        if (menuItem == 0) {
            display.println(cities[i]);
        } else {
            display.print("Ton ");
            display.println(i + 1);
        }
    }

    display.setTextColor(0x4208);
    display.setCursor(2, 110);
    display.println("Kurz=Nav  Lang=Speich.");
}

void showVolumeOverlay(Adafruit_ST7735& display, int volume) {
    display.fillRect(68, 0, 60, 20, ST77XX_BLACK);
    display.setTextColor(ST77XX_WHITE);
    display.setTextSize(1);
    display.setCursor(72, 5);
    display.print("Vol:");
    display.print(volume);
    display.print("/30");
    display.setTextColor(ST77XX_BLACK);

    volumeOverlayActive = true;
    volumeOverlayShownAt = millis();
}

void updateVolumeOverlay() {
    if (!volumeOverlayActive) return;
    if (millis() - volumeOverlayShownAt > VOLUME_OVERLAY_DURATION_MS) {
        display.fillRect(68, 0, 60, 20, ST77XX_WHITE);
        volumeOverlayActive = false;
    }
}

void scrollMenuUp() {
    if (currentMode == MODE_MENU) {
        menuItem = (menuItem - 1 + 3) % 3;
        showMenu(display);
    } else if (currentMode == MODE_EDIT) {
        int maxItems = (menuItem == 0) ? numCities : (menuItem == 1) ? numAthanTones : numReminderTones;
        menuScroll = (menuScroll - 1 + maxItems) % maxItems;
        showEditScreen(display);
    }
}

void scrollMenuDown() {
    if (currentMode == MODE_MENU) {
        menuItem = (menuItem + 1) % 3;
        showMenu(display);
    } else if (currentMode == MODE_EDIT) {
        int maxItems = (menuItem == 0) ? numCities : (menuItem == 1) ? numAthanTones : numReminderTones;
        menuScroll = (menuScroll + 1) % maxItems;
        showEditScreen(display);
    }
}

void confirmSelection() {
    if (currentMode == MODE_MENU) {
        currentMode = MODE_EDIT;
        menuScroll = 0;
        showEditScreen(display);
        Serial.print("-> MODE_EDIT Menuepunkt: ");
        Serial.println(menuItem);
    } else if (currentMode == MODE_EDIT) {
        saveAndExit();
    }
}

void saveAndExit() {
    switch (menuItem) {
        case 0:
            selectedCity = cities[menuScroll];
            Serial.print("Stadt gespeichert: ");
            Serial.println(selectedCity);
            break;
        case 1:
            athanTone = String(menuScroll);
            Serial.print("Athan gespeichert: Ton ");
            Serial.println(athanTone);
            break;
        case 2:
            reminderTone = String(menuScroll);
            Serial.print("Reminder gespeichert: Ton ");
            Serial.println(reminderTone);
            break;
    }

    currentMode = MODE_HOME;
    display.fillScreen(ST77XX_WHITE);
    Serial.println("Einstellungen gespeichert -> Home");
}