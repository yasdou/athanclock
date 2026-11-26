# athanclock

I am building an athan clock to remind me of my prayers. Maybe you want one too? 

When I was searching for one to buy I only found ugly clocks from 19. century or very new somewhat good locking clocks, that look like they are made from the cheapest plastic.

So I started to build one myself. It is very simple right now. But I hope that the project gets bigger with time. 

I am no Developer. The code is mainly written by ChatGPT. 

I used a NodeMCU (ESP8266 but might change to ESP32 for bluetooth and more power), a DFPlayer Mini, a 4 GB SD Card and a 8 Watt 3 Ohm speaker. 
I will provide schematics if you want to build it for yourself.

Main Goals: 

1. Show the Prayer Times of the day of my city [DONE]
2. Play the Athan when the Prayer is due [DOING]
3. Play a reminder 15 minutes before each prayer [DOING]
4. Show a countdown 15 minutes before each prayer [DOING]
5. Local Website or App to configure city, athan type etc
6. 


Plans for the future: 
1. Connect with Mawaqit to get the times of each respective mosque
2. Make it updateble OTA (best over internet and automatic each night over GitHub)
3. 

Vision: 
1. Make a lightweight and cheap athan clock that looks nice and is of benefit to its users
2. Build and sell it to mosques so they can resell it, so they have an income


Pin Belegung:

D0 -> Button down (D0/GPIO16 hat keinen internen Pull-up: externen 10 kOhm Widerstand nach 3,3 V verwenden)
D1 -> DF Player Mini TX
D2 -> DF Player Mini RX
D3 -> Display A0
D4 -> Display Reset
D5 -> Display SCK
D6 -> Button up
D7 -> Display SDA
D8 -> Display CS

SD-Karte (DFPlayer):

/01/001.mp3 ... 006.mp3 -> Athan-Töne 1-6
/02/001.mp3 ... 006.mp3 -> Erinnerungs-Töne 1-6
/03/001.mp3 ... 006.mp3 -> Fajr-Athan 1-6 (optional; fehlt der Ordner, nutzt Fajr die Töne aus /01)

## Einrichtung (ohne Code)

1. Uhr einschalten. Ohne gespeichertes WLAN öffnet sie den Hotspot `AthanClock-XXXX`.
   Name und Passwort stehen auf dem Display.
2. Mit dem Handy verbinden. Die Einrichtungsseite öffnet sich automatisch (sonst `http://192.168.4.1`).
3. WLAN auswählen, Passwort eingeben, fertig. Danach ist die Uhr unter `http://athanclock.local` erreichbar.

- WLAN später ändern: Webseite → System → WLAN „Ändern“, oder beim Einschalten die Taste UP gedrückt halten.
- Ist das WLAN länger als 5 Minuten weg, öffnet die Uhr zusätzlich den Hotspot und versucht im Hintergrund weiter, sich zu verbinden.
- Das Geräte-Passwort (Hotspot, WLAN ändern, Firmware-Update) wird beim ersten Start zufällig erzeugt und steht im Menü unter „Info“.

## Firmware-Update über WLAN

- Browser: `http://athanclock.local/update`, Benutzer `admin`, Geräte-Passwort, Datei `.pio/build/nodemcu/firmware.bin`.
- PlatformIO: `pio run -e nodemcu_ota -t upload` (Passwort in `platformio.ini` bzw. `PLATFORMIO_UPLOAD_FLAGS`).

## Tests

The time logic (reminders, athan, month change, API URL) lives in `prayer_logic.cpp` and has no Arduino dependencies, so it can be tested on a PC (run inside this folder):

```
g++ -std=c++11 -Wall -Wextra -I. test/test_prayer_logic.cpp prayer_logic.cpp -o test_prayer_logic
./test_prayer_logic
```

The day-by-day parser for the aladhan response (`month_parser.h`) is tested against a real API response in `test/fixtures/`. It needs the header-only ArduinoJson library:

```
g++ -std=c++11 -Wall -Wextra -I. -isystem <path-to-ArduinoJson>/src test/test_month_parser.cpp prayer_logic.cpp -o test_month_parser
./test_month_parser
```

GitHub Actions (`.github/workflows/ci.yml`) runs these tests and compiles the firmware for the NodeMCU on every push.
