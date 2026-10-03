#pragma once
// Optional 0.91" SSD1306 128x32 I2C display in the stand (SDA = D2/GPIO4, SCL = D1/GPIO5, 0x3C).
// Detected at start-up; without it everything else works unchanged.
#include <Wire.h>
#include <U8g2lib.h>

class Screen {
public:
  bool present = false;

  void begin(bool flip) {
    Wire.begin(4, 5);
    Wire.beginTransmission(0x3C);
    present = Wire.endTransmission() == 0;
    if (!present) return;
    u8g2.begin();
    u8g2.setBusClock(400000);
    setFlip(flip);
    u8g2.enableUTF8Print();
    on = true;
  }

  void setFlip(bool flip) {
    if (present && flip != flipped) { flipped = flip; u8g2.setDisplayRotation(flip ? U8G2_R2 : U8G2_R0); last = ""; }
  }
  void setLang(const String &l) { ru = l == "ru"; }

  void off() {
    if (!present || !on) return;
    u8g2.setPowerSave(1);
    on = false; last = "";
  }

  // Burn-in protection: the position walks a figure-eight, one small step per minute, one lap per hour,
  // so every pixel is lit about equally and neighbouring positions differ by only 1-2 px.
  static void orbit(int rx, int ry, int &dx, int &dy) {
    float a = 2 * PI * ((millis() / 60000UL) % 60) / 60.0f;
    dx = lroundf(rx * sinf(a));
    dy = lroundf(ry * sinf(2 * a));
  }

  // Three text lines: small header, medium main line, small footer. Redraws only on change.
  // drift = shown for long (setup portal, errors, "always on"): moves +-3 px / 1 px with the orbit.
  void text(const String &head, const String &main, const String &foot, uint8_t contrast = 160, bool drift = false) {
    if (!present) return;
    int dx = 0, dy = 0;
    if (drift) { orbit(3, 1, dx, dy); if (dy > 0) dy = 0; }   // keep the footer inside the 32 rows
    String key = head + "\x1f" + main + "\x1f" + foot + "\x1f" + contrast + "\x1f" + dx + "," + dy;
    if (on && key == last) return;
    wake(contrast);
    u8g2.firstPage();
    do {
      u8g2.setFont(small()); center(8 + dy, head, dx);
      u8g2.setFont(medium()); center(21 + dy, main, dx);
      u8g2.setFont(small()); center(31 + dy, foot, dx);
    } while (u8g2.nextPage());
    last = key;
  }

  void progress(const String &head, int done, int total) {
    if (!present) return;
    int pct = total > 0 ? (int)((int64_t)done * 100 / total) : 0;
    String key = head + "\x1f%" + pct;
    if (on && key == last) return;
    wake(160);
    String p = String(pct) + " %";
    u8g2.firstPage();
    do {
      u8g2.setFont(small()); center(8, head);
      u8g2.drawFrame(4, 13, 120, 9);
      u8g2.drawBox(6, 15, 116 * pct / 100, 5);
      center(31, p);
    } while (u8g2.nextPage());
    last = key;
  }

  // Exact time in large digits (24 of 32 rows), dimmed, moving on the orbit (+-18 px, +-3 px).
  void clock(int h, int m) {
    if (!present) return;
    char b[6];
    snprintf(b, sizeof(b), "%02d:%02d", h, m);
    int dx, dy;
    orbit(18, 3, dx, dy);
    String key = String("\x1e") + b + "\x1f" + dx + "," + dy;
    if (on && key == last) return;
    wake(12);
    u8g2.setFont(u8g2_font_logisoso24_tn);
    int w = u8g2.getStrWidth(b);
    u8g2.firstPage();
    do { u8g2.drawStr((128 - w) / 2 + dx, 28 + dy, b); } while (u8g2.nextPage());
    last = key;
  }

private:
  U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2{U8G2_R0, U8X8_PIN_NONE};  // page mode: 128 B buffer, heap is precious
  bool on = false, flipped = false, ru = false;
  String last;

  const uint8_t *small() const { return ru ? u8g2_font_5x8_t_cyrillic : u8g2_font_5x8_tf; }
  const uint8_t *medium() const { return ru ? u8g2_font_6x13_t_cyrillic : u8g2_font_6x13_tf; }
  void center(int y, const String &s, int dx = 0) {
    if (!s.length()) return;
    int w = u8g2.getUTF8Width(s.c_str());
    int x = w >= 128 ? 0 : (128 - w) / 2 + dx;
    u8g2.drawUTF8(constrain(x, 0, max(0, 128 - w)), y, s.c_str());
  }
  void wake(uint8_t contrast) {
    if (!on) { u8g2.setPowerSave(0); on = true; }
    u8g2.setContrast(contrast);
  }
};
