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

  // Three text lines: small header, medium main line, small footer. Redraws only on change.
  void text(const String &head, const String &main, const String &foot, uint8_t contrast = 160) {
    if (!present) return;
    String key = head + "\x1f" + main + "\x1f" + foot + "\x1f" + contrast;
    if (on && key == last) return;
    wake(contrast);
    u8g2.firstPage();
    do {
      u8g2.setFont(small()); center(8, head);
      u8g2.setFont(medium()); center(21, main);
      u8g2.setFont(small()); center(31, foot);
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

  // Exact time in large digits; drifts a few pixels every minute against OLED burn-in.
  void clock(int h, int m) {
    if (!present) return;
    char b[6];
    snprintf(b, sizeof(b), "%02d:%02d", h, m);
    String key = String("\x1e") + b;
    if (on && key == last) return;
    wake(12);
    u8g2.setFont(u8g2_font_logisoso24_tn);
    int w = u8g2.getStrWidth(b);
    int dx = (m % 5) * 6 - 12, dy = (m / 5) % 3 - 1;
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
  void center(int y, const String &s) {
    if (!s.length()) return;
    int w = u8g2.getUTF8Width(s.c_str());
    u8g2.drawUTF8(w >= 128 ? 0 : (128 - w) / 2, y, s.c_str());
  }
  void wake(uint8_t contrast) {
    if (!on) { u8g2.setPowerSave(0); on = true; }
    u8g2.setContrast(contrast);
  }
};
