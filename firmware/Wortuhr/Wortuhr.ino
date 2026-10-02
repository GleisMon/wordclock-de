// Wortuhr — German word clock firmware for ESP8266 (Lolin NodeMCU v3)
// https://github.com/GleisMon/wordclock-de
//
// LED data: D4 (GPIO2) through a series resistor. 132 x WS2812B, 12 x 11 grid.
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <coredecls.h>
#include <time.h>

#include "version.h"
#include "grammar.h"
#include "display.h"
#include "settings.h"
#include "gh_update.h"
#include "webui.h"
#include "screen.h"

static const char *HOSTNAME = "wortuhr";
static const char *AP_SETUP = "Wortuhr-Setup";
static const char *AP_RESCUE = "Wortuhr-Rescue";
static const uint32_t PALETTE[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF, 0xFFFFFF, 0xFF7A00};
static const uint8_t PALETTE_N = sizeof(PALETTE) / sizeof(PALETTE[0]);

Display display;
Settings cfg;
GhUpdate gh;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiManager wm;
Screen screen;

enum Anim { A_NONE, A_HEART, A_TEST } anim = A_NONE;  // before the first function (Arduino prototypes)
uint32_t animStart = 0;

// ---------- texts for the optional stand display ----------
enum Txt { S_SETUP, S_WEB, S_NOWIFI, S_RETRY, S_NOTIME, S_RESCUE, S_UPDFAIL, S_UPDAVAIL, S_CONNECT, S_UPLOAD, S_RESTART, S_COUNT };
// one '\0'-separated block per language in flash (string constants would otherwise sit in RAM)
static const char TXT_DE[] PROGMEM =
  "WLAN einrichten:\0Einstellungen im Browser:\0Kein WLAN\0verbinde neu ...\0Keine Uhrzeit\0Rettungsmodus - WLAN:\0"
  "Update fehlgeschlagen\0Update verfügbar: \0verbinde ...\0Firmware-Upload\0Neustart ...";
static const char TXT_EN[] PROGMEM =
  "Set up WiFi:\0Settings in the browser:\0No WiFi\0reconnecting ...\0No time\0Rescue mode - WiFi:\0"
  "Update failed\0Update available: \0connecting ...\0Firmware upload\0restarting ...";
static const char TXT_RU[] PROGMEM =
  "Настройка Wi-Fi:\0Настройки в браузере:\0Нет Wi-Fi\0переподключение ...\0Нет времени\0Аварийный режим, Wi-Fi:\0"
  "Обновление не удалось\0Есть обновление: \0подключение ...\0Загрузка прошивки\0перезагрузка ...";
String T(Txt k) {
  const char *p = cfg.lang == "en" ? TXT_EN : cfg.lang == "ru" ? TXT_RU : TXT_DE;
  for (uint8_t i = 0; i < k; i++) p += strlen_P(p) + 1;
  return String(FPSTR(p));
}

bool rescue = false, routesReady = false, servicesUp = false, portalOn = false, otaReady = false;
uint32_t portalSince = 0, staRetryAt = 0, lostSince = 0, saveAt = 0, nextCheckAt = 0, rebootAt = 0, lastAutoTry = 0;
bool pendingCheck = false, pendingInstall = false, forceRecolor = true;
uint32_t lastMask = 0, lastTick = 0;
int lastHeartHour = -1;
uint32_t wordColor[W_COUNT];
RgbColor wordFrame[NUM_LEDS], frame[NUM_LEDS];


// ---------- rescue mode: 3 boots in a row that die within 30 s ----------
struct RtcBoot { uint32_t magic, count; };
static const uint32_t RTC_MAGIC = 0x57C10C01;
bool bootCounterCleared = false;

uint32_t bumpBootCounter() {
  RtcBoot b;
  ESP.rtcUserMemoryRead(0, (uint32_t *)&b, sizeof(b));
  if (b.magic != RTC_MAGIC) b = {RTC_MAGIC, 0};
  b.count++;
  ESP.rtcUserMemoryWrite(0, (uint32_t *)&b, sizeof(b));
  return b.count;
}
void clearBootCounter() {
  RtcBoot b = {RTC_MAGIC, 0};
  ESP.rtcUserMemoryWrite(0, (uint32_t *)&b, sizeof(b));
}

// ---------- update boot: the firmware download needs ~30 KB heap, so it runs in a reboot
// without web server / mDNS / OTA / portal. Request and result travel through RTC memory.
struct RtcText { uint32_t magic; char text[64]; };
static const uint32_t RTC_UPD_REQ = 0x55504451, RTC_UPD_RES = 0x55504452;
static const uint32_t RTC_OFS_REQ = 2, RTC_OFS_RES = 2 + sizeof(RtcText) / 4, RTC_OFS_CHK = 2 + 2 * sizeof(RtcText) / 4;
static const uint32_t RTC_CHECKING = 0x43484B31;  // set while a version check runs; survives a crash
bool autoCheckBlocked = false;

void rtcPut(uint32_t ofs, uint32_t magic, const String &s) {
  RtcText r = {magic, {0}};
  strncpy(r.text, s.c_str(), sizeof(r.text) - 1);
  ESP.rtcUserMemoryWrite(ofs, (uint32_t *)&r, sizeof(r));
}
bool rtcTake(uint32_t ofs, uint32_t magic, String &out) {
  RtcText r;
  ESP.rtcUserMemoryRead(ofs, (uint32_t *)&r, sizeof(r));
  if (r.magic != magic) return false;
  r.text[sizeof(r.text) - 1] = 0;
  out = r.text;
  r.magic = 0;
  ESP.rtcUserMemoryWrite(ofs, (uint32_t *)&r, sizeof(r));
  return true;
}

// ---------- helpers ----------
RgbColor rgb(uint32_t c) { return RgbColor((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF); }

uint8_t levelFromPct(uint8_t pct) {  // perceptual curve
  if (pct == 0) return 0;
  uint16_t v = (uint32_t)pct * pct * 255 / 10000;
  return v < 1 ? 1 : v;
}

bool inNight(int minuteOfDay) {
  if (!cfg.nightOn || cfg.nightFrom == cfg.nightTo) return false;
  return cfg.nightFrom < cfg.nightTo ? (minuteOfDay >= cfg.nightFrom && minuteOfDay < cfg.nightTo)
                                     : (minuteOfDay >= cfg.nightFrom || minuteOfDay < cfg.nightTo);
}

void fillCorners(RgbColor c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) frame[i] = RgbColor(0);
  frame[ledIndex(0, 0)] = frame[ledIndex(0, GRID_W - 1)] = frame[ledIndex(GRID_H - 1, 0)] = frame[ledIndex(GRID_H - 1, GRID_W - 1)] = c;
}

void statusPulse(uint32_t color) {  // corners breathe: blue = setup portal, orange = waiting for time, red = rescue
  float k = 0.15f + 0.85f * (0.5f + 0.5f * sinf(millis() / 600.0f));
  RgbColor c = rgb(color);
  fillCorners(RgbColor(c.R * k, c.G * k, c.B * k));
  display.setLevel(90);
  display.setFrame(frame);
}

void assignColors(const Phrase &p) {
  uint8_t idx[PALETTE_N];
  for (uint8_t i = 0; i < PALETTE_N; i++) idx[i] = i;
  for (uint8_t attempt = 0; attempt < 16; attempt++) {  // every word gets a new colour, all different
    for (int8_t i = PALETTE_N - 1; i > 0; i--) { uint8_t j = random(i + 1); uint8_t t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
    bool ok = true;
    for (uint8_t i = 0; i < p.n; i++) if (wordColor[p.w[i]] == PALETTE[idx[i]]) { ok = false; break; }
    if (ok) break;
  }
  for (uint8_t i = 0; i < p.n; i++) wordColor[p.w[i]] = cfg.colorMode == 1 ? cfg.color : PALETTE[idx[i % PALETTE_N]];
}

void buildWordFrame(const Phrase &p) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) wordFrame[i] = RgbColor(0);
  for (uint8_t i = 0; i < p.n; i++) {
    const WordPos &w = WORDS[p.w[i]];
    for (uint8_t c = 0; c < w.len; c++) wordFrame[ledIndex(w.row, w.col + c)] = rgb(wordColor[p.w[i]]);
  }
}

// ---------- animations ----------
static const char HEART[GRID_H][GRID_W + 1] = {
  "..XX....XX..", ".XXXX..XXXX.", "XXXXXXXXXXXX", "XXXXXXXXXXXX", "XXXXXXXXXXXX", ".XXXXXXXXXX.",
  "..XXXXXXXX..", "...XXXXXX...", "....XXXX....", ".....XX.....", "............"};

void startAnim(Anim a) { anim = a; animStart = millis(); }

bool renderAnim() {
  uint32_t t = millis() - animStart;
  if (anim == A_HEART) {
    if (t > 4000) return false;
    float ph = fmodf(t / 900.0f, 1.0f);  // lub-dub
    float beat = 0.35f + 0.65f * max(expf(-30 * ph * ph), 0.7f * expf(-30 * (ph - 0.28f) * (ph - 0.28f)));
    for (uint8_t r = 0; r < GRID_H; r++)
      for (uint8_t c = 0; c < GRID_W; c++)
        frame[ledIndex(r, c)] = HEART[r][c] == 'X' ? RgbColor(255 * beat, 0, 20 * beat) : RgbColor(0);
  } else if (anim == A_TEST) {
    if (t > 6000) return false;
    static const uint32_t seq[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
    RgbColor c = rgb(seq[(t / 1500) % 4]);
    for (uint16_t i = 0; i < NUM_LEDS; i++) frame[i] = c;
  } else return false;
  display.setFrame(frame);
  return true;
}

// ---------- the clock ----------
void tick() {
  time_t now = time(nullptr);
  if (now < 1700000000) {  // no valid time yet
    statusPulse(portalOn ? 0x0040FF : 0xFF8000);
    return;
  }
  struct tm t;
  localtime_r(&now, &t);
  bool night = inNight(t.tm_hour * 60 + t.tm_min);
  display.setLevel(levelFromPct(night ? cfg.nightBrightness : cfg.brightness));

  if (lastHeartHour < 0) lastHeartHour = t.tm_hour;
  if (cfg.heart && t.tm_min == 0 && t.tm_hour != lastHeartHour && anim == A_NONE) {
    lastHeartHour = t.tm_hour;
    if (!(night && cfg.nightBrightness == 0)) startAnim(A_HEART);
  }
  if (anim != A_NONE) {
    if (renderAnim()) return;
    anim = A_NONE;
    display.setTarget(wordFrame, cfg.fade ? 600 : 0);
  }

  GrammarOpts o;
  o.minuten = cfg.minuten; o.v20 = cfg.v20; o.v40 = cfg.v40; o.esIstAlways = cfg.esIstAlways;
  Phrase p = buildPhrase(t.tm_hour, t.tm_min, o);
  if (p.mask != lastMask || forceRecolor) {
    assignColors(p);
    buildWordFrame(p);
    display.setTarget(wordFrame, cfg.fade ? 900 : 0);
    lastMask = p.mask;
    forceRecolor = false;
  }
}

// ---------- stand display: what to show, most important first ----------
void screenLoop() {
  if (!screen.present) return;
  if (rescue) { screen.text(T(S_RESCUE), AP_RESCUE, "192.168.4.1/update"); return; }
  if (portalOn) { screen.text(T(S_SETUP), AP_SETUP, "192.168.4.1"); return; }
  time_t now = time(nullptr);
  bool valid = now > 1700000000, up = WiFi.status() == WL_CONNECTED;
  struct tm t = {};
  if (valid) localtime_r(&now, &t);
  bool night = valid && inNight(t.tm_hour * 60 + t.tm_min);
  if (!up && lostSince && millis() - lostSince > 20000) { screen.text(T(S_NOWIFI), WiFi.SSID(), T(S_RETRY), night ? 12 : 160); return; }
  if (up && !valid && millis() > 60000) { screen.text(T(S_NOTIME), "NTP", cfg.ntp, night ? 12 : 160); return; }
  bool infoTime = cfg.oledMin == 255 || millis() < cfg.oledMin * 60000UL;
  if (infoTime && !night) {
    String foot = "wortuhr.local";
    if (gh.error.startsWith("Update")) foot = T(S_UPDFAIL);
    else if (gh.available()) foot = T(S_UPDAVAIL) + gh.latest;
    else if (valid) { char b[8]; snprintf(b, sizeof(b), "  %02d:%02d", t.tm_hour, t.tm_min); foot += b; }
    screen.text(T(S_WEB), up ? WiFi.localIP().toString() : T(S_CONNECT), foot);
    return;
  }
  if (cfg.oledAfter == 1 && valid && !night) { screen.clock(t.tm_hour, t.tm_min); return; }
  screen.off();
}

// ---------- web ----------
void applyConfig(const Settings &old) {
  screen.setFlip(cfg.oledFlip);
  screen.setLang(cfg.lang);
  display.setLimit(cfg.limitMa);
  if (old.tz != cfg.tz || old.ntp != cfg.ntp) configTime(cfg.tz.c_str(), cfg.ntp.c_str(), "pool.ntp.org");
  if (old.colorMode != cfg.colorMode || (cfg.colorMode == 1 && old.color != cfg.color)) forceRecolor = true;
}

void handleState() {
  JsonDocument doc;
  cfg.toJson(doc["cfg"].to<JsonObject>());
  JsonObject st = doc["st"].to<JsonObject>();
  time_t now = time(nullptr);
  st["synced"] = now > 1700000000;
  if (now > 1700000000) {
    struct tm t; localtime_r(&now, &t);
    char b[24];
    strftime(b, sizeof(b), "%H:%M", &t); st["time"] = b;
    strftime(b, sizeof(b), "%d.%m.%Y", &t); st["date"] = b;
  }
  st["ver"] = FW_VERSION;
  st["ip"] = WiFi.localIP().toString();
  st["ssid"] = WiFi.SSID();
  st["rssi"] = WiFi.RSSI();
  st["heap"] = ESP.getFreeHeap();
  st["uptime"] = millis() / 1000;
  st["mA"] = display.lastMa();
  st["checked"] = gh.checked;
  st["latest"] = gh.latest;
  st["updAvail"] = gh.available();
  st["updErr"] = gh.error;
  st["updHeap"] = gh.heapAtConnect;
  st["updBlock"] = gh.blockAtConnect;
  st["oled"] = screen.present;
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleConfig() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "bad json"); return; }
  Settings old = cfg;
  cfg.fromJson(doc.as<JsonObjectConst>());
  applyConfig(old);
  saveAt = millis() + 3000;  // sliders: write flash once they stop moving
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleAction() {
  String a = server.arg("a");
  if (a == "reroll") forceRecolor = true;
  else if (a == "heartnow") startAnim(A_HEART);
  else if (a == "test") startAnim(A_TEST);
  else if (a == "check") pendingCheck = true;
  else if (a == "install") pendingInstall = true;
  else if (a == "reboot") rebootAt = millis() + 500;
  else if (a == "wifireset") { wm.resetSettings(); rebootAt = millis() + 500; }
  else if (a == "factory") { LittleFS.remove("/config.json"); wm.resetSettings(); rebootAt = millis() + 500; }
  else { server.send(400, "text/plain", "unknown action"); return; }
  server.send(200, "application/json", "{\"ok\":true}");
}

void setupRoutes() {
  if (routesReady) return;
  if (rescue) {
    server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", RESCUE_HTML); });
    server.on("/normal", HTTP_GET, [] { clearBootCounter(); server.send(200, "text/plain", "Neustart..."); rebootAt = millis() + 500; });
  } else {
    server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", INDEX_HTML); });
    server.on("/api/state", HTTP_GET, handleState);
    server.on("/api/config", HTTP_POST, handleConfig);
    server.on("/api/action", HTTP_POST, handleAction);
  }
  httpUpdater.setup(&server, "/update");  // Updater only accepts images signed with our key
  server.onNotFound([] { server.sendHeader("Location", "/"); server.send(302); });
  routesReady = true;
}

void startServices() {
  setupRoutes();
  server.begin();
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  if (!otaReady) {
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) { uploadProgress(done, total); });
    ArduinoOTA.begin();
    otaReady = true;
  }
  servicesUp = true;
  if (!nextCheckAt) nextCheckAt = millis() + 60000;
  Serial.printf("[net] %s  http://%s.local  ip %s\n", WiFi.SSID().c_str(), HOSTNAME, WiFi.localIP().toString().c_str());
}

void stopServices() {
  server.stop();
  MDNS.end();
  servicesUp = false;
}

void openPortal() {
  if (servicesUp) stopServices();
  wm.startConfigPortal(rescue ? AP_RESCUE : AP_SETUP);
  portalOn = true;
  portalSince = millis();
  Serial.println("[net] setup portal open");
}

// ---------- WiFi state machine ----------
void wifiLoop() {
  if (portalOn) wm.process();
  bool up = WiFi.status() == WL_CONNECTED;
  if (up) {
    lostSince = 0; staRetryAt = 0;
    if (portalOn) { wm.stopConfigPortal(); portalOn = false; }
    if (!servicesUp) startServices();
    return;
  }
  if (!lostSince) lostSince = millis();
  if (portalOn) {
    // saved network may be back: every 5 min (nobody on the portal) try it for 30 s
    if (millis() - portalSince > 300000 && wm.getWiFiIsSaved() && WiFi.softAPgetStationNum() == 0) {
      wm.stopConfigPortal(); portalOn = false;
      WiFi.mode(WIFI_STA); WiFi.begin();
      staRetryAt = millis();
    }
  } else if (staRetryAt && millis() - staRetryAt > 30000) {
    staRetryAt = 0; openPortal();
  } else if (!staRetryAt && millis() - lostSince > 600000) {
    openPortal();  // lost for 10 min (new router?) -> offer setup again, keep showing time
  }
}

void installUpdate() {  // normal mode: hand over to the update boot
  if (!gh.available()) return;
  MDNS.end();
  String url;
  bool ok = gh.assetUrl(url);  // follow github.com's redirects here, the update boot only talks to the CDN
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  if (!ok) { Serial.printf("[upd] resolve failed: %s\n", gh.error.c_str()); return; }
  File f = LittleFS.open("/upd.url", "w");
  if (!f) { gh.error = "fs"; return; }
  f.print(url);
  f.close();
  Serial.printf("[upd] rebooting to install %s\n", gh.latest.c_str());
  screen.text("Update " + gh.latest, T(S_RESTART), "");
  if (saveAt) cfg.save();
  rtcPut(RTC_OFS_REQ, RTC_UPD_REQ, gh.latest);
  delay(50);
  ESP.restart();
}

void ledProgress(int done, int total) {
  fillCorners(RgbColor(0));
  for (uint16_t i = 0; i < (uint32_t)NUM_LEDS * done / max(total, 1); i++) frame[i] = RgbColor(0, 0, 255);
  display.setLevel(60); display.setFrame(frame); display.loop();
}

void uploadProgress(size_t done, size_t total) {  // web upload / ArduinoOTA
  ledProgress(done, total);
  screen.progress(T(S_UPLOAD), done, total);
}

void showProgress(int done, int total) {  // GitHub update (update boot)
  ledProgress(done, total);
  screen.progress("Update " + gh.latest, done, total);
}

void runUpdateBoot(const String &version) {
  clearBootCounter();  // a deliberate reboot, not a crash
  Serial.printf("[upd] update boot -> %s\n", version.c_str());
  String url;
  if (LittleFS.begin()) {
    File f = LittleFS.open("/upd.url", "r");
    if (f) { url = f.readString(); f.close(); }
    LittleFS.remove("/upd.url");
    cfg.load();  // display orientation and language only
    LittleFS.end();  // give its buffers back before TLS
  }
  display.begin();
  gh.latest = version;
  screen.begin(cfg.oledFlip);
  screen.setLang(cfg.lang);
  screen.text("Update " + version, T(S_CONNECT), "");
  ledProgress(0, 1);
  WiFi.mode(WIFI_STA);
  WiFi.begin();  // credentials saved by WiFiManager
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 40000) { delay(100); statusPulse(0x0040FF); display.loop(); }
  configTime(TZ_BERLIN, "de.pool.ntp.org", "pool.ntp.org");
  t0 = millis();
  while (time(nullptr) < 1700000000 && millis() - t0 < 30000) { delay(100); statusPulse(0x0040FF); display.loop(); }
  gh.latest = version;
  if (!url.startsWith("https://")) gh.error = "no URL";
  else if (WiFi.status() != WL_CONNECTED) gh.error = "no WiFi";
  else {
    Serial.printf("[upd] heap before download %u (max block %u)\n", ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
    gh.install(url, showProgress);  // reboots on success
  }
  Serial.printf("[upd] failed: %s heap@connect=%u block=%u\n", gh.error.c_str(), gh.heapAtConnect, gh.blockAtConnect);
  rtcPut(RTC_OFS_RES, RTC_UPD_RES, gh.error);
  delay(50);
  ESP.restart();
}

// ---------- serial console: "dump" prints every phrase for the current options ----------
void serialConsole() {
  static String line;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch != '\n' && ch != '\r') { if (line.length() < 32) line += ch; continue; }
    if (line == "dump") {
      GrammarOpts o;
      o.minuten = cfg.minuten; o.v20 = cfg.v20; o.v40 = cfg.v40; o.esIstAlways = cfg.esIstAlways;
      for (int h = 0; h < 24; h++)
        for (int m = 0; m < 60; m += 5) {
          Phrase p = buildPhrase(h, m, o);
          Serial.printf("%02d:%02d", h, m);
          for (uint8_t i = 0; i < p.n; i++) Serial.printf(" %s", WORD_NAMES[p.w[i]]);
          Serial.println();
          yield();
        }
      Serial.println("END");
    } else if (line == "info") {
      Serial.printf("%s %s heap=%u mA=%u ip=%s\n", FW_NAME, FW_VERSION, ESP.getFreeHeap(), display.lastMa(), WiFi.localIP().toString().c_str());
    }
    line = "";
  }
}

// ---------- main ----------
void setup() {
  Serial.begin(115200);
  Serial.printf("\n\n%s %s\n", FW_NAME, FW_VERSION);
  String updVersion, updResult;
  if (rtcTake(RTC_OFS_REQ, RTC_UPD_REQ, updVersion)) runUpdateBoot(updVersion);  // never returns
  if (rtcTake(RTC_OFS_RES, RTC_UPD_RES, updResult)) { gh.error = "Update: " + updResult; gh.checked = true; }
  String crashed;
  if (rtcTake(RTC_OFS_CHK, RTC_CHECKING, crashed)) {  // last boot died inside a check: no auto-check this time
    autoCheckBlocked = true;
    gh.error = "Update-Prüfung abgestürzt";
    gh.checked = true;
  }
  rescue = bumpBootCounter() >= 3;
  if (!LittleFS.begin()) { LittleFS.format(); LittleFS.begin(); }
  if (!rescue) cfg.load();
  display.begin();
  display.setLimit(cfg.limitMa);
  screen.begin(cfg.oledFlip);
  screen.setLang(cfg.lang);
  screen.text(FW_NAME " " FW_VERSION, T(S_CONNECT), "");
  Update.onProgress([](size_t done, size_t total) { uploadProgress(done, total); });
  randomSeed(RANDOM_REG32);

  configTime(cfg.tz.c_str(), cfg.ntp.c_str(), "pool.ntp.org");
  settimeofday_cb([](bool from_sntp) { if (from_sntp) Serial.println("[time] NTP sync"); });

  WiFi.hostname(HOSTNAME);
  wm.setHostname(HOSTNAME);
  wm.setTitle(FW_NAME);
  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(30);
  std::vector<const char *> menu = {"wifi", "info", "exit"};
  wm.setMenu(menu);

  if (rescue) {
    Serial.println("[rescue] 3 failed boots -> rescue mode");
    statusPulse(0xFF0000); display.loop();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_RESCUE);
    WiFi.begin();  // also reachable via the home network if it works
    setupRoutes();
    server.begin();
    MDNS.begin(HOSTNAME);
    return;
  }

  statusPulse(0xFF8000); display.loop();
  if (wm.autoConnect(AP_SETUP)) startServices();
  else { portalOn = true; portalSince = millis(); }
}

void loop() {
  if (!bootCounterCleared && millis() > 30000) { clearBootCounter(); bootCounterCleared = true; }
  if (rebootAt && millis() > rebootAt) ESP.restart();

  if (rescue) {
    server.handleClient();
    MDNS.update();
    if (millis() - lastTick > 30) { lastTick = millis(); statusPulse(0xFF0000); screenLoop(); }
    display.loop();
    return;
  }

  serialConsole();
  wifiLoop();
  if (servicesUp) { server.handleClient(); MDNS.update(); ArduinoOTA.handle(); }

  if (saveAt && millis() > saveAt) { cfg.save(); saveAt = 0; }

  if (servicesUp && (pendingCheck || (!autoCheckBlocked && nextCheckAt && millis() > nextCheckAt))) {
    pendingCheck = false;
    nextCheckAt = millis() + 24UL * 3600 * 1000;
    MDNS.end();  // TLS needs every free byte
    rtcPut(RTC_OFS_CHK, RTC_CHECKING, "");
    gh.check();
    String dummy; rtcTake(RTC_OFS_CHK, RTC_CHECKING, dummy);
    MDNS.begin(HOSTNAME);
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[upd] latest=%s err=%s heap@connect=%u block=%u heap=%u\n", gh.latest.c_str(), gh.error.c_str(), gh.heapAtConnect, gh.blockAtConnect, ESP.getFreeHeap());
  }
  if (servicesUp && pendingInstall) { pendingInstall = false; installUpdate(); }
  if (servicesUp && cfg.autoUpdate && gh.available() && millis() - lastAutoTry > 3600000UL) {
    time_t now = time(nullptr); struct tm t; localtime_r(&now, &t);
    if (t.tm_hour == 3) { lastAutoTry = millis(); installUpdate(); }
  }

  if (millis() - lastTick > 30) { lastTick = millis(); tick(); }
  static uint32_t lastScreen = 0;
  if (millis() - lastScreen > 250) { lastScreen = millis(); screenLoop(); }
  display.loop();
}
