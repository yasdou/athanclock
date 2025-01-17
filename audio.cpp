#include "audio.h"

// Definiere die Pins für die serielle Kommunikation mit dem DFPlayer Mini
#define RX_PIN D1 // RX des DFPlayer Mini
#define TX_PIN D2 // TX des DFPlayer Mini

// Globale Variablen
SoftwareSerial mySerial(RX_PIN, TX_PIN); // SoftwareSerial für den DFPlayer Mini
DFRobotDFPlayerMini myDFPlayer;
bool isAudioInitialized = false; // Zustand der Audio-Initialisierung

void setupAudio() {
    const int maxRetries = 3; // Maximale Anzahl von Versuchen
    int retryCount = 0;

    mySerial.begin(9600); // DFPlayer Mini arbeitet mit 9600 Baud

    while (retryCount < maxRetries) {
        Serial.printf("Versuche DFPlayer Mini zu initialisieren... (Versuch %d von %d)\n", retryCount + 1, maxRetries);
        if (myDFPlayer.begin(mySerial)) {
            showBootMessage("DFPlayer Mini erfolgreich initialisiert.");
            Serial.println("DFPlayer Mini erfolgreich initialisiert.");
            myDFPlayer.volume(20); // Lautstärke einstellen (0-30)
            isAudioInitialized = true;
            break;
        }
        retryCount++;
        delay(1000); // Warte 1 Sekunde vor dem nächsten Versuch
    }

    if (!isAudioInitialized) {
        showBootMessage("Fehler: Kein Adhan bis reboot.");
        delay(3000);
        Serial.println("Fehler: DFPlayer Mini konnte nicht initialisiert werden. Fahre ohne Audio fort.");
    }
}

void playBoot() {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfügbar. Kein Boot-Ton.");
        return;
    }
    myDFPlayer.volume(30);
    Serial.println("Spiele Boot ab...");
    myDFPlayer.play(2); // Spiele die zweite Datei auf der SD-Karte
}

void playAthan() {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfügbar. Kein Athan.");
        return;
    }
    myDFPlayer.volume(15);
    Serial.println("Spiele Athan ab...");
    myDFPlayer.play(1); // Spiele die erste Datei auf der SD-Karte
}

void playReminder() {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfügbar. Kein Reminder.");
        return;
    }
    myDFPlayer.volume(30);
    Serial.println("Spiele Reminder ab...");
    myDFPlayer.play(2); // Spiele die dritte Datei auf der SD-Karte
}
