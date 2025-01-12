#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Display initialisieren
void setupDisplay(Adafruit_ST7735& display) {
    display.initR(INITR_BLACKTAB); // Initialisiere das Display mit dem BlackTab-Profil
    display.fillScreen(ST77XX_BLACK); // Lösche den Bildschirm (schwarz)
    display.setRotation(0); // Hochformat (0 = Standard-Hochformat)
    display.setTextWrap(true); // Textumbruch aktivieren
}

// Display aktualisieren
void updateDisplay(Adafruit_ST7735& display, String fajr, String dhuhr, String asr, String maghrib, String isha, String time) {
    display.fillScreen(ST77XX_BLACK); // Bildschirm löschen
    display.setTextColor(ST77XX_WHITE); // Textfarbe Weiß
    display.setTextSize(1); // Schriftgröße 1

    // Gebetszeiten anzeigen
    display.setCursor(0, 0);
    display.print("Fajr: "); 
    display.println(fajr);

    display.setCursor(0, 15);
    display.print("Dhuhr: "); 
    display.println(dhuhr);

    display.setCursor(0, 30);
    display.print("Asr: "); 
    display.println(asr);

    display.setCursor(0, 45);
    display.print("Maghrib: "); 
    display.println(maghrib);

    display.setCursor(0, 60);
    display.print("Isha: "); 
    display.println(isha);

    // Uhrzeit anzeigen
    display.setCursor(0, 80);
    display.print("Uhrzeit: ");
    display.println(time);

    // Änderungen anzeigen
    display.display();
}
