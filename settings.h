// settings.h
// Speichert Stadt, Töne, Modi und Lautstärke dauerhaft im Flash (LittleFS),
// damit sie einen Neustart überleben.
#ifndef SETTINGS_H
#define SETTINGS_H

void loadSettings();         // einmal in setup() vor setupAudio() aufrufen
void saveSettingsIfDirty();  // in loop() aufrufen; schreibt 2 s nach der letzten Änderung
// markSettingsDirty() ist in config.h deklariert

#endif
