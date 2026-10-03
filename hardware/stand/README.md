# Ständer mit verstecktem NodeMCU

> **Welche Datei?** Die Word Clock gibt es in zwei Größen. **v8** passt zur großen Version (LED-Streifen
> 60 LED/m): Uhr 230 mm breit, 17 mm dick, Schraubenlöcher 212 mm auseinander, 9 mm über der Unterkante.
> **v4/v5** passen zur kleinen Version aus `hardware/clock-case` (74 LED/m): 187 mm breit, 15 mm dick, Löcher 173 mm.
> Vor dem Druck die eigene Uhr nachmessen und zuerst `wortuhr-stand_v8_testslice.stl` (22-mm-Endstück) anprobieren.

- `wortuhr-stand_v8.stl` / `.step` – große Uhr (60 LED/m), mit OLED-Fenster wie v5; Länge 237 mm. USB-C-Ausschnitt rechteckig
  14,2 × 5,9 mm, symmetrisch (Buchse 14 × 5,2; die runde 5,3-mm-Variante passte nicht), 2,5 mm tiefer als in v7 und weiter hinten, ganz über dem Schraubenblock (keine Aussparung im Block → keine
  Überhänge beim Druck mit dem Bildschirm nach unten); Übergang Boden → Auflage als gerade Fase 2 mm auf 6 mm; OLED-Tasche mit 2-mm-Rand innen
  (45°-Übergang in die Wand), Einstecktiefe 3,7 mm. Die STLs liegen für den Druck **Bildschirmseite nach unten**
  (schräge Rückwand auf dem Bett).
- `wortuhr-stand_v8_testslice.stl` – rechtes Endstück (Schraubenblock, USB-C-Buchse) zum Anprobieren.

- `wortuhr-stand_v5.stl` / `.step` – wie v4, zusätzlich Fenster für einen 0,91"-OLED (SSD1306 128×32) oben in der
  schrägen Rückwand: Tasche 39 × 12,2 × 1,7 mm von innen, Fenster 24 × 8,2 mm mit Fase 0,5 mm, 0,8 mm Haut,
  Wand überall 2,5 mm (kein Rahmen; das Modul steht innen ~0,8 mm über). Maße aus dem Reflex-Deckel.
  Modul mit einem Tropfen Kleber oder Klebeband sichern. Außerdem: keine Fasen an den Stirnkanten, die an der
  Uhr anliegen (Rückseite, Auflage, Vorderkante bündig mit der Front), 6-mm-Fase an der oberen Innenkante der
  Schraubenblöcke.
- `wortuhr-stand_v4.stl` – druckfertig, liegt so auf dem Druckbett, wie er auf dem Tisch steht (keine Stützen nötig)
- `wortuhr-stand_v4.step` – zum Weiterbearbeiten
- `fusion-scripts/` – Fusion-360-Skripte, die das Modell erzeugen (`v4_build.py`, `v5_chamfer.py`, `v5_screen.py`) und die Passung prüfen (`chk_v4.py`, `chk_v5.py`)

**Maße (v4/v5):** 194 × 50 × 56 mm; v8: 237 × 53 × 56 mm. Die Fusion-Skripte erzeugen v8 (Konstanten oben in `v4_build.py`), Uhr 10° nach hinten geneigt, Vorderseite bündig mit der Front der Uhr.

**Befestigung:** zwei Schrauben Ø 3,7 × 40 mm (Kopf Ø 5,9 × 2 mm) durch den Ständer in die beiden unteren
Löcher der Uhr. Klemmlänge 30 mm (Rückseite der Uhr bis Senkungsboden), Senkung Ø 6,6 × 2,5 mm,
Durchgang Ø 4,2 mm → die Schraube greift ca. 10 mm in den Dom der Uhr (verfügbar: 13 mm).
Unterlegscheibe M3 empfohlen.

**Innenraum:** NodeMCU v3 (58 × 31 mm) passt stehend hinter die Uhr. Rechts (von vorne) ein Ausschnitt
13,9 × 5,3 mm (abgerundet) für eine USB-C-Einbaubuchse 13,7 × 5,1 mm, Wand dort 1,5 mm für die Rastnasen.

Druck: PETG, 0,2 mm, Fasen 45° statt Rundungen.

**Lizenz:** CC BY-NC-SA 4.0 – angelehnt an „Stand for Word Clock“ von **LarsIversen** (MakerWorld, CC BY-NC-SA).
https://creativecommons.org/licenses/by-nc-sa/4.0/
