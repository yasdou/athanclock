#include "menu.h"
#include "display.h"
#include "audio.h"
#include "config.h"

extern Adafruit_ST7735 display;
extern String selectedCity;
extern String athanTone;
extern String reminderTone;
extern String fajrTime;
extern String shurukTime;
extern String dhuhrTime;
extern String asrTime;
extern String maghribTime;
extern String ishaTime;
String getFormattedTime();

AppMode currentMode = MODE_HOME;
int menuItem = 0;
int menuScroll = 0;

unsigned long volumeOverlayShownAt = 0;
bool volumeOverlayActive = false;
const unsigned long VOLUME_OVERLAY_DURATION_MS = 2000;

unsigned long lastMenuActivity = 0;
const unsigned long MENU_TIMEOUT_MS = 5000;

String cities[] = {"Mainz", "Berlin", "Muenchen", "Hamburg", "Koeln"};
int numCities = 5;
int numAthanTones = 6;
int numReminderTones = 6;

static void drawHomeNow() {
    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextWrap(true);
    display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
    display.fillScreen(ST77XX_WHITE);
    updateDisplay(display, fajrTime, shurukTime, dhuhrTime, asrTime, maghribTime, ishaTime, getFormattedTime());
}

bool isMenuOpen() {
    return currentMode == MODE_MENU || currentMode == MODE_EDIT;
}

void menuRegisterActivity() {
    lastMenuActivity = millis();
}

void closeMenuToHome() {
    stopAudio();
    volumeOverlayActive = false;
    currentMode = MODE_HOME;
    drawHomeNow();
    Serial.println("-> MODE_HOME");
}

void toggleMenu() {
    if (currentMode == MODE_HOME) {
        currentMode = MODE_MENU;
        menuItem = 0;
        menuRegisterActivity();
        showMenu(display);
        Serial.println("-> MODE_MENU");
    } else {
        closeMenuToHome();
    }
}

void updateMenuTimeout() {
    if (!isMenuOpen()) return;
    if (millis() - lastMenuActivity > MENU_TIMEOUT_MS) {
        closeMenuToHome();
    }
}

void showMenu(Adafruit_ST7735& display) {
    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextWrap(true);
    display.fillScreen(ST77XX_WHITE);

    display.fillRect(0, 0, 128, 16, ST77XX_BLUE);
    display.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    display.setCursor(30, 4);
    display.println("=  MENU  =");

    const char* items[] = {"1. Stadt", "2. Athan-Ton", "3. Reminder-Ton"};
    for (int i = 0; i < 3; i++) {
        int yPos = 25 + i * 18;
        if (i == menuItem) {
            display.fillRect(0, yPos - 2, 128, 16, ST77XX_CYAN);
            display.setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        } else {
            display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
        }
        display.setCursor(8, yPos);
        display.println(items[i]);
    }

    display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
    display.setCursor(2, 110);
    display.println("Kurz=Nav  Lang=Ausw.");
}

void showEditScreen(Adafruit_ST7735& display) {
    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextWrap(true);
    display.fillScreen(ST77XX_WHITE);

    display.fillRect(0, 0, 128, 16, ST77XX_BLUE);
    display.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    display.setCursor(5, 4);
    switch (menuItem) {
        case 0: display.println("  Stadt waehlen");  break;
        case 1: display.println("  Athan-Ton");      break;
        case 2: display.println("  Reminder-Ton");   break;
    }

    int maxItems = (menuItem == 0) ? numCities : (menuItem == 1) ? numAthanTones : numReminderTones;
    int visibleStart = max(0, min(menuScroll, maxItems - 4));

    for (int i = visibleStart; i < min(maxItems, visibleStart + 4); i++) {
        int yPos = 25 + (i - visibleStart) * 18;
        if (i == menuScroll) {
            display.fillRect(0, yPos - 2, 128, 16, ST77XX_CYAN);
            display.setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        } else {
            display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
        }
        display.setCursor(8, yPos);
        if (menuItem == 0) {
            display.println(cities[i]);
        } else {
            display.print("Ton ");
            display.println(i + 1);
        }
    }

    display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
    display.setCursor(2, 110);
    display.println("Kurz=Nav Lang=Speich.");
}

void showVolumeOverlay(Adafruit_ST7735& display, int volume) {
    display.fillRect(68, 0, 60, 20, ST77XX_BLACK);
    display.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display.setTextSize(1);
    display.setCursor(72, 5);
    display.print("Vol:");
    display.print(volume);
    display.print("/30");
    volumeOverlayActive = true;
    volumeOverlayShownAt = millis();
}

void updateVolumeOverlay() {
    if (!volumeOverlayActive) return;
    if (millis() - volumeOverlayShownAt > VOLUME_OVERLAY_DURATION_MS) {
        volumeOverlayActive = false;
        if (isMenuOpen()) {
            if (currentMode == MODE_EDIT) showEditScreen(display);
            else showMenu(display);
        } else {
            drawHomeNow();
        }
    }
}

void previewCurrentTone() {
    if (menuItem == 1) {
        stopAudio();
        delay(80);
        playAthan(String(menuScroll));
        Serial.print("Vorschau Athan Ton: ");
        Serial.println(menuScroll + 1);
    } else if (menuItem == 2) {
        stopAudio();
        delay(80);
        playReminder(String(menuScroll));
        Serial.print("Vorschau Reminder Ton: ");
        Serial.println(menuScroll + 1);
    }
}

void scrollMenuUp() {
    menuRegisterActivity();
    if (currentMode == MODE_MENU) {
        menuItem = (menuItem - 1 + 3) % 3;
        showMenu(display);
    } else if (currentMode == MODE_EDIT) {
        int maxItems = (menuItem == 0) ? numCities : (menuItem == 1) ? numAthanTones : numReminderTones;
        menuScroll = (menuScroll - 1 + maxItems) % maxItems;
        showEditScreen(display);
        if (menuItem == 1 || menuItem == 2) previewCurrentTone();
    }
}

void scrollMenuDown() {
    menuRegisterActivity();
    if (currentMode == MODE_MENU) {
        menuItem = (menuItem + 1) % 3;
        showMenu(display);
    } else if (currentMode == MODE_EDIT) {
        int maxItems = (menuItem == 0) ? numCities : (menuItem == 1) ? numAthanTones : numReminderTones;
        menuScroll = (menuScroll + 1) % maxItems;
        showEditScreen(display);
        if (menuItem == 1 || menuItem == 2) previewCurrentTone();
    }
}

void confirmSelection() {
    menuRegisterActivity();
    if (currentMode == MODE_MENU) {
        currentMode = MODE_EDIT;

        if (menuItem == 0) {
            int currentIndex = 0;
            for (int i = 0; i < numCities; i++) {
                if (cities[i] == selectedCity) {
                    currentIndex = i;
                    break;
                }
            }
            menuScroll = currentIndex;
        } else if (menuItem == 1) {
            menuScroll = athanTone.toInt();
        } else if (menuItem == 2) {
            menuScroll = reminderTone.toInt();
        }

        showEditScreen(display);
        if (menuItem == 1 || menuItem == 2) previewCurrentTone();
        Serial.print("-> MODE_EDIT Menuepunkt: ");
        Serial.println(menuItem);
    } else if (currentMode == MODE_EDIT) {
        saveSelection();
    }
}

void goBackOneLevel() {
    menuRegisterActivity();
    if (currentMode == MODE_EDIT) {
        stopAudio();
        currentMode = MODE_MENU;
        showMenu(display);
        Serial.println("-> Zurueck zu MODE_MENU");
    } else if (currentMode == MODE_MENU) {
        closeMenuToHome();
    }
}

void saveSelection() {
    menuRegisterActivity();
    stopAudio();
    delay(80);

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
            playAthan(athanTone);
            break;
        case 2:
            reminderTone = String(menuScroll);
            Serial.print("Reminder gespeichert: Ton ");
            Serial.println(reminderTone);
            playReminder(reminderTone);
            break;
    }

    currentMode = MODE_MENU;
    showMenu(display);
    Serial.println("Gespeichert -> Zurueck ins Menue");
}