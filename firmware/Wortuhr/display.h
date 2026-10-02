#pragma once
// LED output: cross-fades, global brightness and a hard current limit for the USB supply.
#include <NeoPixelBus.h>
#include "grammar.h"

class Display {
public:
  // Data on D4 (GPIO2) = hardware UART1 -> no interrupt blocking, WiFi stays happy.
  NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> strip{NUM_LEDS};

  void begin() {
    strip.Begin();
    strip.ClearTo(RgbColor(0));
    strip.Show();
  }

  void setLimit(uint16_t mA) { limitMa = mA; dirty = true; }
  void setLevel(uint8_t lvl) { if (lvl != level) { level = lvl; dirty = true; } }

  // Fade from whatever is shown now to `target`.
  void setTarget(const RgbColor *target, uint16_t ms) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) { from[i] = cur[i]; to[i] = target[i]; }
    fadeMs = ms; fadeStart = millis(); fading = ms > 0;
    if (!fading) for (uint16_t i = 0; i < NUM_LEDS; i++) cur[i] = to[i];
    dirty = true;
  }

  // Immediate frame (animations, status); cancels a running fade.
  void setFrame(const RgbColor *frame) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) cur[i] = frame[i];
    fading = false; dirty = true;
  }

  const RgbColor *current() const { return cur; }
  uint16_t lastMa() const { return lastCurrentMa; }

  void loop() {
    if (fading) {
      uint32_t t = millis() - fadeStart;
      if (t >= fadeMs) { for (uint16_t i = 0; i < NUM_LEDS; i++) cur[i] = to[i]; fading = false; }
      else {
        uint8_t k = (uint32_t)t * 255 / fadeMs;
        for (uint16_t i = 0; i < NUM_LEDS; i++) cur[i] = RgbColor::LinearBlend(from[i], to[i], k);
      }
      dirty = true;
    }
    if (!dirty || !strip.CanShow()) return;
    dirty = false;
    // brightness, then current limit (WS2812B: ~20 mA per channel at full duty)
    uint32_t sum = 0;
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      RgbColor c = cur[i].Dim(level);
      sum += (uint32_t)c.R + c.G + c.B;
    }
    uint32_t ma = sum * 20 / 255;
    uint8_t scale = 255;
    if (limitMa && ma > limitMa) scale = (uint32_t)limitMa * 255 / ma;
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      RgbColor c = cur[i].Dim(level);
      strip.SetPixelColor(i, scale < 255 ? c.Dim(scale) : c);
    }
    lastCurrentMa = (uint32_t)ma * scale / 255;
    strip.Show();
  }

private:
  RgbColor cur[NUM_LEDS], from[NUM_LEDS], to[NUM_LEDS];
  uint32_t fadeStart = 0;
  uint16_t fadeMs = 0, limitMa = 1500, lastCurrentMa = 0;
  uint8_t level = 128;
  bool fading = false, dirty = true;
};
