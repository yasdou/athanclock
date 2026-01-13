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
    
    bool btnUp = !digitalRead(BTN_VOL_UP);
    bool btnDn = !digitalRead(BTN_VOL_DN);
    
    if (btnUp || btnDn) {
        lastBtnPress = now;
        
        // Audio stoppen bei beliebigem Button
        if (isAudioPlaying()) {
            stopAudio();
            return;
        }
        
        if (btnPressTime == 0) btnPressTime = now;
        
        // Lange Taste (2s) → Menü
        if (btnUp && (now - btnPressTime > 2000)) {
            toggleMenu();
            btnPressTime = 0;
        }
    } else {
        if (btnPressTime > 0) {
            unsigned long duration = now - btnPressTime;
            if (duration < 2000) {
                if (!digitalRead(BTN_VOL_UP)) onShortPressUp();
                if (!digitalRead(BTN_VOL_DN)) onShortPressDown();
            }
            btnPressTime = 0;
        }
    }
}

bool isAnyButtonPressed() {
    return !digitalRead(BTN_VOL_UP) || !digitalRead(BTN_VOL_DN);
}
