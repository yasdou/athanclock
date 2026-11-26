#include "menu.h"

#include <ESP8266WiFi.h>

#include "audio.h"
#include "config.h"
#include "display.h"
#include "html.h"
#include "prayer_logic.h"
#include "wifi_setup.h"

extern Adafruit_ST7735 display;

AppMode currentMode = MODE_HOME;
int menuItem = 0;
int menuScroll = 0;

// Menüpunkte
enum MenuEntry { MENU_CITY = 0, MENU_ATHAN, MENU_FAJR, MENU_REMINDER, MENU_INFO, MENU_COUNT };
static const char* const kMenuLabels[MENU_COUNT] = {"Ort", "Athan", "Fajr-Athan", "Erinnerung", "Info"};

// Feste Orte mit Koordinaten (eindeutig, keine Namenssuche nötig).
// Weitere Orte lassen sich über die Ortssuche der Webseite wählen.
struct MenuCity {
  const char* name;
  const char* region;
  double latitude;
  double longitude;
};
static const MenuCity kCities[] = {
    {"Mainz", "Rheinland-Pfalz", 49.98185, 8.28008},
    {"Berlin", "Berlin", 52.52437, 13.41053},
    {"M\xC3\xBCnchen", "Bayern", 48.13743, 11.57549},  // München (UTF-8)
    {"Hamburg", "Hamburg", 53.55073, 9.99302},
    {"K\xC3\xB6ln", "Nordrhein-Westfalen", 50.93333, 6.95},  // Köln (UTF-8)
};
static const int numCities = sizeof(kCities) / sizeof(kCities[0]);

static const int kToneCount = 6;

unsigned long volumeOverlayUntil = 0;
unsigned long lastMenuActionMillis = 0;
const unsigned long MENU_TIMEOUT_MS = 5000;
const unsigned long INFO_TIMEOUT_MS = 30000;  // Zeit zum Abschreiben des Passworts

static String displayName(const String& utf8) {
  char buf[64];
  prayer::transliterateForDisplay(utf8.c_str(), buf, sizeof(buf));
  return String(buf);
}

// Zurück zum Startbildschirm. Gezeichnet wird in loop() (render()),
// damit dort immer Countdown oder Zeiten korrekt erscheinen.
static void drawHomeScreen() {
  currentMode = MODE_HOME;
  requestScreenRedraw();
}

static void markMenuActivity() { lastMenuActionMillis = millis(); }

static int getCurrentToneIndex(const String& tone) {
  int idx = tone.toInt();
  if (idx < 0) idx = 0;
  if (idx >= kToneCount) idx = kToneCount - 1;
  return idx;
}

static bool isToneEntry(int item) { return item == MENU_ATHAN || item == MENU_FAJR || item == MENU_REMINDER; }

static String* toneFor(int item) {
  if (item == MENU_ATHAN) return &athanTone;
  if (item == MENU_FAJR) return &fajrTone;
  if (item == MENU_REMINDER) return &reminderTone;
  return nullptr;
}

static void showMessage(const String& line1, const String& line2) {
  display.fillScreen(ST77XX_WHITE);
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);
  display.setCursor(5, 50);
  display.println(line1);
  display.setCursor(5, 65);
  display.println(line2);
}

bool isMenuOpen() { return currentMode == MODE_MENU || currentMode == MODE_EDIT; }

void toggleMenu() {
  if (currentMode == MODE_HOME) {
    currentMode = MODE_MENU;
    menuItem = 0;
    markMenuActivity();
    showMenu(display);
    Serial.println("-> MODE_MENU");
  } else {
    drawHomeScreen();
    Serial.println("-> MODE_HOME");
  }
}

void showMenu(Adafruit_ST7735& display) {
  display.fillScreen(ST77XX_WHITE);
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);

  display.setCursor(10, 5);
  display.println("=== MENU ===");

  for (int i = 0; i < MENU_COUNT; i++) {
    display.setCursor(5, 22 + i * 14);
    display.print(i == menuItem ? "> " : "  ");
    display.print(i + 1);
    display.print(". ");
    display.print(kMenuLabels[i]);
  }

  display.setCursor(5, 106);
  display.println("Kurz: Navigieren");
  display.setCursor(5, 120);
  display.println("Lang UP: Auswahl");
  display.setCursor(5, 134);
  display.println("Lang DN: Zurueck");
}

static void showInfoScreen(Adafruit_ST7735& display) {
  display.setCursor(30, 5);
  display.println("== Info ==");

  display.setCursor(4, 22);
  display.print("Version ");
  display.print(FIRMWARE_VERSION);

  String wifi = savedWifiSsid();
  if (wifi.length() > 15) wifi = wifi.substring(0, 14) + "~";
  display.setCursor(4, 36);
  display.print("WLAN: ");
  display.print(wifi.length() ? wifi : String("-"));

  display.setCursor(4, 50);
  if (isSetupMode()) {
    display.print("Hotspot aktiv");
  } else {
    display.print("IP: ");
    display.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("-"));
  }
  display.setCursor(4, 64);
  display.print(WEB_HOSTNAME ".local");

  display.setCursor(4, 84);
  display.print("Geraete-Passwort:");
  display.setTextSize(2);
  display.setCursor(16, 98);
  display.print(devicePassword());
  display.setTextSize(1);

  display.setCursor(4, 128);
  display.print("Fuer Update & WLAN");
  display.setCursor(4, 144);
  display.print("Lang DN: Zurueck");
}

void showEditScreen(Adafruit_ST7735& display) {
  display.fillScreen(ST77XX_WHITE);
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(ST77XX_BLACK);

  if (menuItem == MENU_INFO) {
    showInfoScreen(display);
    return;
  }

  if (menuItem == MENU_CITY) {
    display.setCursor(8, 5);
    display.println("== Ort waehlen ==");
    for (int i = 0; i < numCities; i++) {
      display.setCursor(5, 25 + i * 15);
      display.println((i == menuScroll ? "> " : "  ") + displayName(kCities[i].name));
    }
    return;
  }

  static const char* const titles[MENU_COUNT] = {"", "== Athan waehlen ==", "== Fajr-Athan ==",
                                                 "= Erinnerung =", ""};
  display.setCursor(4, 5);
  display.println(titles[menuItem]);
  for (int i = 0; i < kToneCount; i++) {
    display.setCursor(5, 25 + i * 15);
    display.println((i == menuScroll ? "> Ton " : "  Ton ") + String(i + 1));
  }
}

static void previewTone() {
  stopAudio();
  if (menuItem == MENU_ATHAN) playAthan(String(menuScroll));
  if (menuItem == MENU_FAJR) playFajrAthan(String(menuScroll), fajrVolume);
  if (menuItem == MENU_REMINDER) playReminder(String(menuScroll));
}

static int editCount() {
  if (menuItem == MENU_CITY) return numCities;
  if (isToneEntry(menuItem)) return kToneCount;
  return 1;
}

static void scrollMenu(int delta) {
  markMenuActivity();

  if (currentMode == MODE_MENU) {
    menuItem = (menuItem + delta + MENU_COUNT) % MENU_COUNT;
    showMenu(display);
    return;
  }

  if (currentMode == MODE_EDIT) {
    if (menuItem == MENU_INFO) return;
    int n = editCount();
    menuScroll = (menuScroll + delta + n) % n;
    if (isToneEntry(menuItem)) previewTone();
    showEditScreen(display);
  }
}

void scrollMenuUp() { scrollMenu(-1); }

void scrollMenuDown() { scrollMenu(1); }

void confirmSelection() {
  markMenuActivity();

  if (currentMode == MODE_MENU) {
    currentMode = MODE_EDIT;
    menuScroll = 0;
    if (menuItem == MENU_CITY) {
      for (int i = 0; i < numCities; i++) {
        if (currentLocation.name == kCities[i].name) {
          menuScroll = i;
          break;
        }
      }
    } else if (isToneEntry(menuItem)) {
      menuScroll = getCurrentToneIndex(*toneFor(menuItem));
      previewTone();
    }
    showEditScreen(display);
    return;
  }

  if (currentMode == MODE_EDIT && menuItem != MENU_INFO) {
    saveSelection();
  }
}

void saveSelection() {
  markMenuActivity();

  if (menuItem == MENU_CITY) {
    const MenuCity& c = kCities[menuScroll];
    if (currentLocation.name != c.name || !currentLocation.hasCoords) {
      showMessage("Lade Zeiten fuer", displayName(c.name) + "...");
      Location location = {c.name, c.region, c.latitude, c.longitude, true};
      bool ok = changeLocation(location, calcMethod, asrSchool);  // übernimmt den Ort nur bei Erfolg
      if (!ok) {
        showMessage("Fehler beim Laden.", "Bleibe bei " + displayName(currentLocation.name));
        delay(2000);
      }
      markMenuActivity();  // der Abruf kann einige Sekunden dauern
    }
    Serial.print("Ort: ");
    Serial.println(currentLocation.name);
  } else if (isToneEntry(menuItem)) {
    *toneFor(menuItem) = String(menuScroll);
    markSettingsDirty();
    Serial.printf("%s gespeichert: Ton %d\n", kMenuLabels[menuItem], menuScroll + 1);
    stopAudio();
  }

  currentMode = MODE_MENU;
  showMenu(display);
}

void saveAndExit() {
  saveSelection();
  drawHomeScreen();
  Serial.println("Einstellungen gespeichert -> Home");
}

void goBackOneLevel() {
  markMenuActivity();

  if (currentMode == MODE_EDIT) {
    stopAudio();
    currentMode = MODE_MENU;
    showMenu(display);
    Serial.println("Zurueck zu MODE_MENU");
    return;
  }

  if (currentMode == MODE_MENU) {
    drawHomeScreen();
    Serial.println("Zurueck zu MODE_HOME");
  }
}

void showVolumeOverlay(Adafruit_ST7735& display, int volume) {
  display.fillRect(0, 120, 128, 8, ST77XX_WHITE);
  display.setCursor(10, 120);
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(1);
  display.print("Vol: ");
  display.print(volume);
  volumeOverlayUntil = millis() + 1500;
  if (volumeOverlayUntil == 0) volumeOverlayUntil = 1;  // 0 heißt "kein Overlay"
}

void updateVolumeOverlay() {
  if (volumeOverlayUntil == 0) return;
  if ((long)(millis() - volumeOverlayUntil) < 0) return;

  volumeOverlayUntil = 0;

  if (currentMode == MODE_MENU) {
    showMenu(display);
  } else if (currentMode == MODE_EDIT) {
    showEditScreen(display);
  } else {
    requestScreenRedraw();
  }
}

void updateMenuTimeout() {
  if (!isMenuOpen()) return;
  unsigned long timeout = (currentMode == MODE_EDIT && menuItem == MENU_INFO) ? INFO_TIMEOUT_MS : MENU_TIMEOUT_MS;
  if (millis() - lastMenuActionMillis < timeout) return;

  stopAudio();
  drawHomeScreen();
  Serial.println("Menue-Timeout -> MODE_HOME");
}
