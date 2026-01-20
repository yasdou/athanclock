#include "audio.h"
#include "config.h"


// Definiere die Pins für die serielle Kommunikation mit dem DFPlayer Mini
#define RX_PIN D1 // RX des DFPlayer Mini
#define TX_PIN D2 // TX des DFPlayer Mini
// Globale Variablen
SoftwareSerial mySerial(RX_PIN, TX_PIN); // SoftwareSerial für den DFPlayer Mini
DFRobotDFPlayerMini myDFPlayer;
bool isAudioInitialized = false; // Zustand der Audio-Initialisierung
bool audioCurrentlyPlaying = false;

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

void playAthan(String athanTone) {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfügbar. Kein Athan.");
        return;
    }

    // Konvertiere die Athan-Ton-Auswahl (z.B. "0", "1", ..., "10") in eine Zahl
    int athanTrack = athanTone.toInt();  // Konvertiere String zu int

    if (athanTrack < 0 || athanTrack > 10) {
        Serial.println("Ungültiger Athan-Ton. Standardton wird verwendet.");
        athanTrack = 1;  // Standardton, wenn der Wert ungültig ist
    }

    myDFPlayer.volume(15);  // Lautstärke auf 15 setzen
    Serial.print("Spiele Athan-Ton ab: ");
    Serial.println(athanTrack);

      // ZU ORDNER 01 WECHSELN
    myDFPlayer.playFolder(1, athanTrack+1);  // Ordner 01, Track X
    audioCurrentlyPlaying = true;
}

void playReminder(String reminderTone) {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfügbar. Kein Reminder.");
        return;
    }

    // Konvertiere die Athan-Ton-Auswahl (z.B. "0", "1", ..., "10") in eine Zahl
    int reminderTrack = reminderTone.toInt();  // Konvertiere String zu int

    if (reminderTrack < 0 || reminderTrack > 10) {
        Serial.println("Ungültiger Athan-Ton. Standardton wird verwendet.");
        reminderTrack = 1;  // Standardton, wenn der Wert ungültig ist
    }

    myDFPlayer.volume(15);  // Lautstärke auf 15 setzen
    Serial.print("Spiele Reminder-Ton ab: ");
    Serial.println(reminderTrack);

    // ZU ORDNER 02 WECHSELN
    myDFPlayer.playFolder(2, reminderTrack+1);  // Ordner 02, Track X
    audioCurrentlyPlaying = true;
}

bool isAudioPlaying() {
    return audioCurrentlyPlaying;
}

void stopAudio() {
    if (!isAudioInitialized) return;
    myDFPlayer.stop();
    audioCurrentlyPlaying = false;
    Serial.println("Audio gestoppt");
}