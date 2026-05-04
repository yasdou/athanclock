#include "buttons.h"
#include "menu.h"
#include "audio.h"

extern Adafruit_ST7735 display;

unsigned long btnPressTimeUp = 0;
unsigned long btnPressTimeDn = 0;
unsigned long lastBtnEvent = 0;
const unsigned long DEBOUNCE_MS = 50;
const unsigned long LONG_PRESS_MS = 2000;

bool upLongHandled = false;
bool dnLongHandled = false;

void setupButtons() {
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DN, INPUT_PULLUP);
}

void handleButtons() {
    unsigned long now = millis();
    if (now - lastBtnEvent < DEBOUNCE_MS) return;

    bool upPressed = !digitalRead(BTN_VOL_UP);
    bool dnPressed = !digitalRead(BTN_VOL_DN);

    if ((upPressed || dnPressed) && isAudioPlaying() && !isMenuOpen()) {
        stopAudio();
        lastBtnEvent = now;
        btnPressTimeUp = 0;
        btnPressTimeDn = 0;
        upLongHandled = false;
        dnLongHandled = false;
        return;
    }

    if (upPressed) {
        if (btnPressTimeUp == 0) {
            btnPressTimeUp = now;
            upLongHandled = false;
        }
        if (!upLongHandled && (now - btnPressTimeUp >= LONG_PRESS_MS)) {
            if (isMenuOpen()) {
                confirmSelection();
            } else {
                toggleMenu();
            }
            upLongHandled = true;
            lastBtnEvent = now;
        }
    } else if (btnPressTimeUp > 0) {
        if (!upLongHandled && (now - btnPressTimeUp < LONG_PRESS_MS)) {
            if (isMenuOpen()) {
                scrollMenuUp();
            } else {
                volumeUp();
                showVolumeOverlay(display, getCurrentVolume());
            }
        }
        btnPressTimeUp = 0;
        upLongHandled = false;
        lastBtnEvent = now;
    }

    if (dnPressed) {
        if (btnPressTimeDn == 0) {
            btnPressTimeDn = now;
            dnLongHandled = false;
        }
        if (!dnLongHandled && (now - btnPressTimeDn >= LONG_PRESS_MS)) {
            if (isMenuOpen()) {
                goBackOneLevel();
            }
            dnLongHandled = true;
            lastBtnEvent = now;
        }
    } else if (btnPressTimeDn > 0) {
        if (!dnLongHandled && (now - btnPressTimeDn < LONG_PRESS_MS)) {
            if (isMenuOpen()) {
                scrollMenuDown();
            } else {
                volumeDown();
                showVolumeOverlay(display, getCurrentVolume());
            }
        }
        btnPressTimeDn = 0;
        dnLongHandled = false;
        lastBtnEvent = now;
    }
}

bool isAnyButtonPressed() {
    return !digitalRead(BTN_VOL_UP) || !digitalRead(BTN_VOL_DN);
}