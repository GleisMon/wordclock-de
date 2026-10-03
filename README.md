# Wortuhr – Firmware für die deutsche Word Clock (ESP8266)

Firmware für eine 3D-gedruckte Wortuhr mit deutschem Ziffernblatt (12 × 11 Buchstaben, 132 WS2812B-LEDs),
gesteuert von einem **Lolin NodeMCU v3 (ESP8266)**, der unsichtbar im Ständer sitzt.

[Русская версия](README.ru.md)

- Einrichtung ohne Programmieren: WLAN per Handy, Zeit per Internet (NTP), Sommerzeit automatisch
- Zufällige Farben – bei jeder neuen Anzeige bekommt **jedes** Wort eine neue Farbe
- Helligkeit, Nachtmodus, „MINUTEN“ ein/aus, Herz zur vollen Stunde (optional)
- Einstellungsseite im Browser: **http://wortuhr.local** (Deutsch / Русский / English)
- Updates über WLAN direkt aus GitHub Releases, signiert; Rettungsmodus falls etwas schiefgeht
- Strombegrenzung für ein 2-A-USB-Netzteil
- Optional: kleiner Bildschirm (0,91" OLED) im Ständer – zeigt Adresse, WLAN-Einrichtung, Fehler,
  Update-Fortschritt und auf Wunsch die genaue Uhrzeit

## Erste Inbetriebnahme

1. Uhr mit dem USB-C-Netzteil verbinden. Die vier Eck-LEDs pulsieren **blau**: die Uhr wartet auf WLAN-Daten.
2. Mit dem Handy das WLAN **„Wortuhr-Setup“** wählen. Die Einrichtungsseite öffnet sich (sonst `http://192.168.4.1`).
3. „Configure WiFi“ → eigenes WLAN wählen, Passwort eingeben, speichern.
4. Ecken pulsieren **orange**, bis die Uhrzeit aus dem Internet geholt ist – dann erscheint die Zeit.

Einstellungen danach im Browser unter **http://wortuhr.local** (oder über die IP-Adresse aus dem Router).

| Ecken pulsieren | Bedeutung |
|---|---|
| blau | Einrichtungs-WLAN „Wortuhr-Setup“ ist offen |
| orange | verbindet sich / wartet auf die Uhrzeit |
| rot | Rettungsmodus (siehe unten) |

Ist das WLAN 10 Minuten weg (z. B. neuer Router), öffnet die Uhr wieder „Wortuhr-Setup“ und zeigt
weiter die Zeit an.

## Wie die Zeit angezeigt wird

| Minute | Anzeige (Standard) |
|---|---|
| :00 | ES IST DREI UHR (bei 1 Uhr: ES IST EIN UHR) |
| :05 / :10 | ES IST FÜNF / ZEHN MINUTEN NACH DREI |
| :15 | ES IST VIERTEL NACH DREI |
| :20 | ES IST ZWANZIG MINUTEN NACH DREI *(oder: ZEHN VOR HALB VIER)* |
| :25 | ES IST FÜNF VOR HALB VIER |
| :30 | ES IST HALB VIER |
| :35 | ES IST FÜNF NACH HALB VIER |
| :40 | ES IST ZWANZIG MINUTEN VOR VIER *(oder: ZEHN NACH HALB VIER)* |
| :45 | ES IST VIERTEL VOR VIER |
| :50 / :55 | ES IST ZEHN / FÜNF MINUTEN VOR VIER |

„MINUTEN“ lässt sich abschalten, die Varianten für :20 / :40 und „ES IST nur zur vollen und halben Stunde“
stehen unter *Erweiterte Einstellungen*.

## Bildschirm im Ständer (optional)

Wird beim Start automatisch erkannt – ohne Bildschirm läuft dieselbe Firmware unverändert.

| Situation | Anzeige |
|---|---|
| WLAN-Einrichtung | „WLAN einrichten: Wortuhr-Setup · 192.168.4.1“ (solange das Portal offen ist) |
| nach dem Einschalten | IP-Adresse, wortuhr.local und Uhrzeit – für die eingestellte Zeit (Standard 5 min, 0 = aus, „immer“) |
| danach | aus (Standard) oder **genaue Uhrzeit** (gedimmt, wandert auf einer Acht-Bahn gegen Einbrennen: 1 Schritt/min, 1 Runde/h) |
| Fehler | „Kein WLAN“, „Keine Uhrzeit“, „Update fehlgeschlagen“ |
| Update | Fortschrittsbalken mit Prozent (GitHub, Browser-Upload, Arduino-OTA) |
| Nachtmodus | aus (Fehler werden gedimmt angezeigt) |

Einstellungen: *Bildschirm im Ständer* auf der Einstellungsseite (erscheint nur mit Bildschirm), dort auch „Bildschirm drehen“.

## Updates

Die Uhr schaut einmal am Tag nach, ob es auf GitHub eine neue Version gibt, und zeigt sie auf der
Einstellungsseite an („Update installieren“). Auf Wunsch installiert sie Updates nachts um 3 Uhr selbst.
Es werden nur Firmware-Dateien angenommen, die mit dem Schlüssel dieses Projekts signiert sind.

**Rettungsmodus:** Startet die Uhr dreimal hintereinander innerhalb von 30 Sekunden neu, öffnet sie das
WLAN **„Wortuhr-Rescue“** (Ecken rot). Dort unter `http://192.168.4.1/update` die Datei `wortuhr-de.bin`
aus dem neuesten [Release](https://github.com/GleisMon/wordclock-de/releases) hochladen.

## Hardware

| Teil | Hinweis |
|---|---|
| Gehäuse + Front | [`hardware/clock-case`](hardware/clock-case) – deutsches Layout von KS, Remix der Word Clock von johniak |
| Ständer | [`hardware/stand`](hardware/stand) – versteckt den NodeMCU, wird mit den zwei unteren Gehäuseschrauben befestigt (v4 ohne, v5 mit Bildschirmfenster) |
| Bildschirm (optional) | SSD1306 0,91" 128×32, I2C 0x3C: **SDA → D2 (GPIO4), SCL → D1 (GPIO5)**, VCC 3,3 V, GND |
| Controller | Lolin NodeMCU v3 (ESP8266, 4 MB Flash) |
| LEDs | WS2812B-Streifen 74 LED/m, 11 Reihen × 12, Schlangenlinie, Anfang unten rechts |
| Strom | 5 V / 2 A USB-Netzteil über USB-C-Einbaubuchse (nur Strom) |

Verdrahtung: LED **DIN → D4 (GPIO2)** über einen Widerstand (z. B. 330 Ω), LED 5 V und NodeMCU **VIN** direkt an
5 V der USB-C-Buchse, GND gemeinsam. Hinweis: Eine 2-polige USB-C-Buchse braucht 5,1-kΩ-CC-Widerstände,
sonst liefern C-auf-C-Netzteile keine Spannung.

## Entwicklung

Arduino-CLI, Board **NodeMCU 1.0 (ESP-12E)**, Flash-Layout **4MB (FS:1MB OTA:~1019KB)**:

```bash
arduino-cli core install esp8266:esp8266@3.1.2
arduino-cli lib install "NeoPixelBus by Makuna@2.8.4" "WiFiManager@2.0.17" "ArduinoJson@7.4.3" "U8g2@2.36.19"
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2:eesz=4M1M,xtal=160,ip=lm2f,ssl=all --output-dir build firmware/Wortuhr
```

- **Signatur:** `firmware/Wortuhr/public.key` ist eingebaut; die Firmware akzeptiert per OTA/Upload nur
  Images, die mit dem passenden `private.key` signiert sind. Der private Schlüssel liegt **nicht** im Repo
  (lokal neben dem Sketch, in CI als Secret `SIGNING_KEY`). Per USB lässt sich jede Firmware flashen.
- **Release:** `FW_VERSION` in `firmware/Wortuhr/version.h` erhöhen, committen, Tag `vX.Y.Z` pushen –
  GitHub Actions baut, signiert und veröffentlicht `wortuhr-de.bin` + `version.json`.
- **Serielle Konsole** (115200 Bd): `dump` gibt alle 144 Sätze für die aktuellen Einstellungen aus, `info` den Status.
- **Zertifikate:** Die Root-CAs für den GitHub-Download stehen in `certs.h`; erneuern mit `tools/make_certs.py`.

## Herkunft und Lizenzen

- **Word Clock** (Original, Gehäuse und Idee): [johniak – Word Clock auf MakerWorld](https://makerworld.com/en/models/686196),
  Code: [github.com/johniak/word-clock](https://github.com/johniak/word-clock)
- **Deutsche Version** (Gehäuse/Front mit deutschem Layout): [KS – Word Clock (German version) auf MakerWorld](https://makerworld.com/en/models/1222379),
  Lizenz **CC BY-SA 4.0** – die Datei in `hardware/clock-case` ist unverändert übernommen.
- **Ständer:** eigene Konstruktion, angelehnt an „Stand for Word Clock“ von **LarsIversen** (MakerWorld, CC BY-NC-SA) –
  daher ebenfalls **CC BY-NC-SA 4.0**.
- **Firmware** (`firmware/`, `tools/`): neu geschrieben, **MIT** (siehe [LICENSE](LICENSE)). Übernommen wurde nur
  die Buchstaben- und LED-Belegung der deutschen Front.
