#include "audio.h"

#include "config.h"

// Pins für die serielle Kommunikation mit dem DFPlayer Mini
#define RX_PIN D1  // an TX des DFPlayer Mini
#define TX_PIN D2  // an RX des DFPlayer Mini

SoftwareSerial mySerial(RX_PIN, TX_PIN);
DFRobotDFPlayerMini myDFPlayer;
bool isAudioInitialized = false;
static bool audioCurrentlyPlaying = false;
static int currentVolume = 20;
static int fajrFolderFiles = -1;  // Anzahl Dateien in /03 (eigene Fajr-Athans), -1 = unbekannt

// Ordner auf der SD-Karte
static const int FOLDER_ATHAN = 1;     // /01/001.mp3 ...
static const int FOLDER_REMINDER = 2;  // /02/001.mp3 ...
static const int FOLDER_FAJR = 3;      // /03/001.mp3 ... (optional, Fajr-Athan mit "as-salatu khayrun min an-naum")

static int clampVolume(int v) { return v < 0 ? 0 : (v > 30 ? 30 : v); }

static void playFromFolder(int folder, int track, int volume, const char* what) {
  if (!isAudioInitialized) {
    Serial.printf("DFPlayer Mini nicht verfügbar. Kein %s.\n", what);
    return;
  }
  int v = volume < 0 ? currentVolume : clampVolume(volume);
  Serial.printf("Spiele %s: Ordner %02d, Datei %03d, Lautstärke %d\n", what, folder, track, v);
  myDFPlayer.volume(v);
  myDFPlayer.playFolder(folder, track);
  audioCurrentlyPlaying = true;
}

static int toneToTrack(const String& tone) {
  int track = tone.toInt();
  if (track < 0 || track > 10) {
    Serial.println("Ungültiger Ton. Standardton wird verwendet.");
    track = 0;
  }
  return track + 1;  // Dateien im Ordner beginnen bei 001
}

void setupAudio() {
  const int maxRetries = 3;
  mySerial.begin(9600);  // DFPlayer Mini arbeitet mit 9600 Baud
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.printf("Versuche DFPlayer Mini zu initialisieren... (Versuch %d von %d)\n", attempt, maxRetries);
    if (myDFPlayer.begin(mySerial)) {
      Serial.println("DFPlayer Mini erfolgreich initialisiert.");
      isAudioInitialized = true;
      myDFPlayer.volume(currentVolume);
      fajrFolderFiles = myDFPlayer.readFileCountsInFolder(FOLDER_FAJR);
      Serial.printf("Eigene Fajr-Athans (Ordner 03): %d\n", fajrFolderFiles < 0 ? 0 : fajrFolderFiles);
      return;
    }
    delay(1000);
  }
  showBootMessage("Fehler: Kein Adhan bis reboot.");
  Serial.println("Fehler: DFPlayer Mini konnte nicht initialisiert werden. Fahre ohne Audio fort.");
  delay(3000);
}

void updateAudio() {
  if (!isAudioInitialized) return;
  if (!myDFPlayer.available()) return;
  if (myDFPlayer.readType() == DFPlayerPlayFinished) {
    audioCurrentlyPlaying = false;
  }
}

void playAthan(String athanTone, int volume) {
  playFromFolder(FOLDER_ATHAN, toneToTrack(athanTone), volume, "Athan");
}

void playFajrAthan(String tone, int volume) {
  int track = toneToTrack(tone);
  // Gibt es den gewählten Fajr-Athan in /03, wird er gespielt, sonst der normale Athan.
  if (fajrFolderFiles >= track) {
    playFromFolder(FOLDER_FAJR, track, volume, "Fajr-Athan");
  } else {
    playFromFolder(FOLDER_ATHAN, track, volume, "Fajr-Athan (aus Ordner 01)");
  }
}

void playReminder(String reminderTone, int volume) {
  playFromFolder(FOLDER_REMINDER, toneToTrack(reminderTone), volume, "Erinnerung");
}

bool hasOwnFajrAthans() { return fajrFolderFiles > 0; }

bool isAudioPlaying() { return audioCurrentlyPlaying; }

void stopAudio() {
  if (!isAudioInitialized) return;
  myDFPlayer.stop();
  audioCurrentlyPlaying = false;
  Serial.println("Audio gestoppt");
}

// Setzt die Lautstärke (0-30). Wird auch vor setupAudio() aus den
// gespeicherten Einstellungen aufgerufen.
void setVolume(int volume) {
  if (volume < 0) volume = 0;
  if (volume > 30) volume = 30;
  if (volume == currentVolume) return;
  currentVolume = volume;
  if (isAudioInitialized) myDFPlayer.volume(currentVolume);
  markSettingsDirty();
}

void volumeUp() { setVolume(currentVolume + 1); }

void volumeDown() { setVolume(currentVolume - 1); }

int getCurrentVolume() { return currentVolume; }
