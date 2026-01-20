#include "buttons.h"
#include "menu.h"
#include "audio.h"

unsigned long btnPressTime = 0;
unsigned long lastBtnPress = 0;

void setupButtons() {
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DN, INPUT_PULLUP);
}

void handleButtons() {
    unsigned long now = millis();
    if (now - lastBtnPress < 50) return;  // Debounce
    
    bool btnUpPressed = !digitalRead(BTN_VOL_UP);
    bool btnDnPressed = !digitalRead(BTN_VOL_DN);
    
    // WICHTIG: Nur bei BUTTON-DRÜCKEN handeln
    if (btnUpPressed || btnDnPressed) {
        // AUDIO STOPPEN bei jedem Drücken
        if (isAudioPlaying()) {
            stopAudio();
            return;
        }
        
        // Lange Taste nur für BTN_VOL_UP!
        if (btnUpPressed && (now - btnPressTime > 2000)) {
            toggleMenu();
            btnPressTime = 0;
            return;
        }
        
        // Button gerade gedrückt (btnPressTime == 0)
        if (btnPressTime == 0) {
            btnPressTime = now;
        }
    } else {
        // LOSGELASSEN → Kurzdruck
        if (btnPressTime > 0) {
            unsigned long duration = now - btnPressTime;
            if (duration < 2000) {  // < 2s = Kurz
                if (digitalRead(BTN_VOL_UP) == HIGH) {
                    onShortPressUp();
                }
                if (digitalRead(BTN_VOL_DN) == HIGH) {
                    onShortPressDown();
                }
            }
            btnPressTime = 0;
        }
    }
}

bool isAnyButtonPressed() {
    return !digitalRead(BTN_VOL_UP) || !digitalRead(BTN_VOL_DN);
}
