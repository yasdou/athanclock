#include "buttons.h"
#include "menu.h"
#include "audio.h"

unsigned long btnPressTime[2] = {0, 0};  // Separate Timestamps für beide Buttons
unsigned long lastBtnPress = 0;

void setupButtons() {
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DN, INPUT_PULLUP);
}

void handleButtons() {
    unsigned long now = millis();
    if (now - lastBtnPress < 50) return;  // Debounce 50ms

    bool btnUpPressed = !digitalRead(BTN_VOL_UP);
    bool btnDnPressed = !digitalRead(BTN_VOL_DN);
    
    // Audio stoppen bei jedem Drücken
    if ((btnUpPressed || btnDnPressed) && isAudioPlaying()) {
        stopAudio();
        lastBtnPress = now;
        return;
    }
    
    // Button UP (langes Drücken = Menü)
    if (btnUpPressed) {
        if (btnPressTime[0] == 0) {  // Erstes Drücken
            btnPressTime[0] = now;
        } else if (now - btnPressTime[0] > 2000) {  // >2s = Lang
            toggleMenu();
            btnPressTime[0] = 0;
            lastBtnPress = now;
            return;
        }
    } else if (btnPressTime[0] > 0) {  // Losgelassen
        unsigned long duration = now - btnPressTime[0];
        if (duration < 2000) {  // Kurzdruck
            onShortPressUp();
        }
        btnPressTime[0] = 0;
        lastBtnPress = now;
        return;
    }
    
    // Button DOWN (nur Kurzdruck)
    if (btnDnPressed) {
        if (btnPressTime[1] == 0) {
            btnPressTime[1] = now;
        }
    } else if (btnPressTime[1] > 0) {
        unsigned long duration = now - btnPressTime[1];
        if (duration < 2000) {
            onShortPressDown();
        }
        btnPressTime[1] = 0;
        lastBtnPress = now;
        return;
    }
}

bool isAnyButtonPressed() {
    return !digitalRead(BTN_VOL_UP) || !digitalRead(BTN_VOL_DN);
}

void onShortPressUp() {
    Serial.println("Volume UP");
    int currentVol = getCurrentVolume();
    if (currentVol < 30) {
        setVolume(currentVol + 1);
    }
}

void onShortPressDown() {
    Serial.println("Volume DOWN");
    int currentVol = getCurrentVolume();
    if (currentVol > 0) {
        setVolume(currentVol - 1);
    }
}