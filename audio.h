#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

extern SoftwareSerial mySerial;
extern DFRobotDFPlayerMini myDFPlayer;
extern bool isAudioInitialized;

void setupAudio();
void updateAudio();  // in loop() aufrufen: erkennt das Ende eines Tracks
// volume: 0-30, -1 = aktuelle Lautstärke
void playAthan(String athanTone, int volume = -1);
void playFajrAthan(String tone, int volume = -1);  // aus /03, sonst /01
void playReminder(String reminderTone, int volume = -1);
bool hasOwnFajrAthans();  // Gibt es Dateien in /03?
void showBootMessage(const char* message);
bool isAudioPlaying();
void stopAudio();
void setVolume(int volume);
void volumeUp();
void volumeDown();
int getCurrentVolume();

#endif
