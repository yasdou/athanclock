#include <AudioFileSourceSPIFFS.h>
#include <AudioOutputI2S.h>
#include <AudioGeneratorWAV.h>

void playAthan() {
    AudioFileSourceSPIFFS file("/athan.wav"); // Datei auf SPIFFS speichern
    AudioOutputI2S out;                      // Audioausgabe über I2S
    AudioGeneratorWAV wav;                   // WAV-Datei abspielen

    if (wav.begin(&file, &out)) {
        while (wav.isRunning()) {
            wav.loop();
        }
        wav.stop();
    } else {
        Serial.println("Fehler beim Starten des Audioplayers!");
    }
}
