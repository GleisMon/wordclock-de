#pragma once
// User settings, persisted as /config.json on LittleFS.
#include <LittleFS.h>
#include <ArduinoJson.h>

#define TZ_BERLIN "CET-1CEST,M3.5.0,M10.5.0/3"

struct Settings {
  uint8_t brightness = 60;        // %
  bool nightOn = false;
  uint16_t nightFrom = 22 * 60;   // minutes after midnight
  uint16_t nightTo = 7 * 60;
  uint8_t nightBrightness = 10;   // %, 0 = display off
  bool minuten = true;
  uint8_t v20 = 0, v40 = 0;
  bool esIstAlways = true;
  bool heart = false;             // heart animation at every full hour
  uint8_t colorMode = 0;          // 0 = random colours per word, 1 = single colour
  uint32_t color = 0xFFB060;      // single colour (warm white)
  bool fade = true;
  String tz = TZ_BERLIN;
  String ntp = "de.pool.ntp.org";
  uint16_t limitMa = 1500;        // LED current budget (2 A supply - ESP8266 reserve)
  bool autoUpdate = false;
  String lang = "de";             // web UI language
  uint8_t oledMin = 5;            // stand display: minutes on after power-up (0 = off, 255 = always)
  uint8_t oledAfter = 0;          // after that: 0 = off, 1 = exact time
  bool oledFlip = false;          // module mounted the other way round

  void toJson(JsonObject o) const {
    o["brightness"] = brightness; o["nightOn"] = nightOn; o["nightFrom"] = nightFrom; o["nightTo"] = nightTo;
    o["nightBrightness"] = nightBrightness; o["minuten"] = minuten; o["v20"] = v20; o["v40"] = v40;
    o["esIstAlways"] = esIstAlways; o["heart"] = heart; o["colorMode"] = colorMode; o["color"] = color;
    o["fade"] = fade; o["tz"] = tz; o["ntp"] = ntp; o["limitMa"] = limitMa; o["autoUpdate"] = autoUpdate; o["lang"] = lang;
    o["oledMin"] = oledMin; o["oledAfter"] = oledAfter; o["oledFlip"] = oledFlip;
  }

  void fromJson(JsonObjectConst o) {
    brightness = constrain((int)(o["brightness"] | brightness), 1, 100);
    nightOn = o["nightOn"] | nightOn;
    nightFrom = (uint16_t)(o["nightFrom"] | nightFrom) % 1440;
    nightTo = (uint16_t)(o["nightTo"] | nightTo) % 1440;
    nightBrightness = constrain((int)(o["nightBrightness"] | nightBrightness), 0, 100);
    minuten = o["minuten"] | minuten;
    v20 = (o["v20"] | v20) ? 1 : 0;
    v40 = (o["v40"] | v40) ? 1 : 0;
    esIstAlways = o["esIstAlways"] | esIstAlways;
    heart = o["heart"] | heart;
    colorMode = (o["colorMode"] | colorMode) ? 1 : 0;
    color = (o["color"] | color) & 0xFFFFFF;
    fade = o["fade"] | fade;
    if (o["tz"].is<const char *>() && strlen(o["tz"]) > 0 && strlen(o["tz"]) < 60) tz = o["tz"].as<const char *>();
    if (o["ntp"].is<const char *>() && strlen(o["ntp"]) > 0 && strlen(o["ntp"]) < 60) ntp = o["ntp"].as<const char *>();
    limitMa = constrain((int)(o["limitMa"] | limitMa), 300, 3000);
    autoUpdate = o["autoUpdate"] | autoUpdate;
    int om = o["oledMin"] | (int)oledMin;
    oledMin = om >= 255 ? 255 : constrain(om, 0, 60);
    oledAfter = (o["oledAfter"] | oledAfter) ? 1 : 0;
    oledFlip = o["oledFlip"] | oledFlip;
    if (o["lang"].is<const char *>()) { String l = o["lang"].as<const char *>(); if (l == "de" || l == "ru" || l == "en") lang = l; }
  }

  void load() {
    File f = LittleFS.open("/config.json", "r");
    if (!f) return;
    JsonDocument doc;
    if (!deserializeJson(doc, f)) fromJson(doc.as<JsonObjectConst>());
    f.close();
  }

  void save() const {
    File f = LittleFS.open("/config.json", "w");
    if (!f) return;
    JsonDocument doc;
    toJson(doc.to<JsonObject>());
    serializeJson(doc, f);
    f.close();
  }
};
