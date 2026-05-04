#include "audio.h"
#include "config.h"

#define RX_PIN D1
#define TX_PIN D2

SoftwareSerial mySerial(RX_PIN, TX_PIN);
DFRobotDFPlayerMini myDFPlayer;
bool isAudioInitialized = false;
bool audioCurrentlyPlaying = false;
int currentVolume = 15;
unsigned long audioStartMillis = 0;
unsigned long audioDurationEstimateMs = 0;

void setupAudio() {
    const int maxRetries = 3;
    int retryCount = 0;
    mySerial.begin(9600);

    while (retryCount < maxRetries) {
        Serial.printf("Versuche DFPlayer Mini zu initialisieren... (Versuch %d von %d)\n", retryCount + 1, maxRetries);
        if (myDFPlayer.begin(mySerial)) {
            Serial.println("DFPlayer Mini erfolgreich initialisiert.");
            currentVolume = 15;
            myDFPlayer.volume(currentVolume);
            isAudioInitialized = true;
            break;
        }
        retryCount++;
        delay(1000);
    }

    if (!isAudioInitialized) {
        Serial.println("Fehler: DFPlayer Mini konnte nicht initialisiert werden. Fahre ohne Audio fort.");
    }
}

void setVolume(int volume) {
    if (!isAudioInitialized) return;
    if (volume < 0) volume = 0;
    if (volume > 30) volume = 30;

    currentVolume = volume;
    myDFPlayer.volume(currentVolume);

    Serial.print("Lautstaerke gesetzt auf: ");
    Serial.println(currentVolume);
}

void volumeUp() {
    setVolume(currentVolume + 1);
}

void volumeDown() {
    setVolume(currentVolume - 1);
}

int getCurrentVolume() {
    return currentVolume;
}

bool isAudioPlaying() {
    if (!isAudioInitialized) return false;

    // Empfohlen: später BUSY-Pin benutzen
    // Beispiel:
    // #define BUSY_PIN D0
    // return digitalRead(BUSY_PIN) == LOW;

    if (!audioCurrentlyPlaying) return false;

    if (millis() - audioStartMillis > audioDurationEstimateMs) {
        audioCurrentlyPlaying = false;
        return false;
    }

    return true;
}

void stopAudio() {
    if (!isAudioInitialized) return;

    myDFPlayer.stop();
    audioCurrentlyPlaying = false;
    audioDurationEstimateMs = 0;

    Serial.println("Audio gestoppt");
}

void playAthan(String athanTone) {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfuegbar. Kein Athan.");
        return;
    }

    if (isAudioPlaying()) {
        Serial.println("Audio laeuft bereits, Athan wird uebersprungen.");
        return;
    }

    int athanTrack = athanTone.toInt();
    if (athanTrack < 0 || athanTrack > 10) {
        Serial.println("Ungueltiger Athan-Ton. Standardton wird verwendet.");
        athanTrack = 0;
    }

    myDFPlayer.volume(currentVolume);
    delay(20);

    Serial.print("Spiele Athan-Ton ab: Ordner 01, Track ");
    Serial.println(athanTrack + 1);

    myDFPlayer.playFolder(1, athanTrack + 1);

    audioCurrentlyPlaying = true;
    audioStartMillis = millis();
    audioDurationEstimateMs = 10UL * 60UL * 1000UL;
}

void playReminder(String reminderTone) {
    if (!isAudioInitialized) {
        Serial.println("DFPlayer Mini nicht verfuegbar. Kein Reminder.");
        return;
    }

    if (isAudioPlaying()) {
        Serial.println("Audio laeuft bereits, Reminder wird uebersprungen.");
        return;
    }

    int reminderTrack = reminderTone.toInt();
    if (reminderTrack < 0 || reminderTrack > 10) {
        Serial.println("Ungueltiger Reminder-Ton. Standardton wird verwendet.");
        reminderTrack = 0;
    }

    myDFPlayer.volume(currentVolume);
    delay(20);

    Serial.print("Spiele Reminder-Ton ab: Ordner 02, Track ");
    Serial.println(reminderTrack + 1);

    myDFPlayer.playFolder(2, reminderTrack + 1);

    audioCurrentlyPlaying = true;
    audioStartMillis = millis();
    audioDurationEstimateMs = 2UL * 60UL * 1000UL;
}