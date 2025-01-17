#ifndef AUDIO_H
#define AUDIO_H

#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

// Externe Deklaration der Variablen
extern SoftwareSerial mySerial;
extern DFRobotDFPlayerMini myDFPlayer;
extern bool isAudioInitialized; // Zustand der Audio-Initialisierung

// Funktionsprototypen
void setupAudio();
void playBoot();
void playAthan();
void playReminder();
void showBootMessage(const char* message);

#endif // AUDIO_H
