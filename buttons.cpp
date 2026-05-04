#include "buttons.h"
#include "menu.h"
#include "audio.h"

// Zugriff auf das globale Display-Objekt aus dem Hauptsketch
extern Adafruit_ST7735 display;

unsigned long btnPressTimeUp = 0;
unsigned long btnPressTimeDn = 0;
unsigned long lastBtnEvent = 0;
const unsigned long DEBOUNCE_MS = 50;
const unsigned long LONG_PRESS_MS = 2000;

void setupButtons() {
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DN, INPUT_PULLUP);
}

void handleButtons() {
    unsigned long now = millis();
    if (now - lastBtnEvent < DEBOUNCE_MS) return;

    bool upPressed = !digitalRead(BTN_VOL_UP);
    bool dnPressed = !digitalRead(BTN_VOL_DN);

    // Audio stoppen bei beliebigem Drücken
    if ((upPressed || dnPressed) && isAudioPlaying()) {
        stopAudio();
        lastBtnEvent = now;
        btnPressTimeUp = 0;
        btnPressTimeDn = 0;
        return;
    }

    // ── BTN_VOL_UP ────────────────────────────────
    if (upPressed) {
        if (btnPressTimeUp == 0) {
            btnPressTimeUp = now;
        }
        // Langer Druck (noch gehalten) → sofort auslösen
        if (now - btnPressTimeUp >= LONG_PRESS_MS) {
            if (isMenuOpen()) {
                confirmSelection();  // Untermenü öffnen oder speichern
            } else {
                toggleMenu();        // Menü öffnen
            }
            btnPressTimeUp = 0;
            lastBtnEvent = now;
        }
    } else if (btnPressTimeUp > 0) {
        // Losgelassen → Kurzdruck
        if (now - btnPressTimeUp < LONG_PRESS_MS) {
            if (isMenuOpen()) {
                scrollMenuUp();      // Menü nach oben navigieren
            } else {
                volumeUp();          // Lautstärke erhöhen
                showVolumeOverlay(display, getCurrentVolume());
            }
        }
        btnPressTimeUp = 0;
        lastBtnEvent = now;
    }

    // ── BTN_VOL_DN ────────────────────────────────
    if (dnPressed) {
        if (btnPressTimeDn == 0) {
            btnPressTimeDn = now;
        }
        // Langer Druck → Bestätigen (im Menü) oder zurück (Home)
        if (now - btnPressTimeDn >= LONG_PRESS_MS) {
            if (isMenuOpen()) {
                confirmSelection();  // Untermenü öffnen oder speichern
            } else {
                // Im Home kein langer DOWN definiert, kann erweitert werden
            }
            btnPressTimeDn = 0;
            lastBtnEvent = now;
        }
    } else if (btnPressTimeDn > 0) {
        // Losgelassen → Kurzdruck
        if (now - btnPressTimeDn < LONG_PRESS_MS) {
            if (isMenuOpen()) {
                scrollMenuDown();    // Menü nach unten navigieren
            } else {
                volumeDown();        // Lautstärke senken
                showVolumeOverlay(display, getCurrentVolume());
            }
        }
        btnPressTimeDn = 0;
        lastBtnEvent = now;
    }
}

bool isAnyButtonPressed() {
    return !digitalRead(BTN_VOL_UP) || !digitalRead(BTN_VOL_DN);
}