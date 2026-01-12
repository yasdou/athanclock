#ifndef AUDIO_H
#define AUDIO_H
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
// Externe Deklaration der Variablen
extern SoftwareSerial mySerial;
extern DFRobotDFPlayerMini myDFPlayer;
extern bool isAudioInitialized; // Zustand der Audio-Initialisierung

extern int athanTrack;
extern int reminderTrack;


// Funktionsprototypen
void setupAudio();
void playAthan(String athanTone);
void playReminder(String reminderTone);
void showBootMessage(const char* message);

#endif // AUDIO_H