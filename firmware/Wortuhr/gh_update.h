#pragma once
// Firmware updates from GitHub Releases.
// Each release carries version.json ({"version":"1.2.3"}) and the signed wortuhr-de.bin.
// Two layers of protection: TLS is verified against the root CAs in certs.h, and the
// Updater only accepts images RSA-signed with our key (public.key, see README).
//
// Heap is the hard limit on the ESP8266: GitHub answers with a redirect to its asset CDN,
// so the redirect is resolved with a small, separate session first and only then the
// CDN session (16 KB records for the download) is opened -- never two at once.
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <memory>
#include <time.h>
#include "version.h"
#include "certs.h"

struct GhUpdate {
  String latest;          // newest version on GitHub, "" = unknown
  String error;
  uint32_t lastCheckMs = 0;
  bool checked = false;
  uint32_t heapAtConnect = 0, blockAtConnect = 0;

  static uint32_t ver(const String &v) {  // "1.2.3" -> 0x010203
    int a = 0, b = 0, c = 0;
    sscanf(v.c_str(), "%d.%d.%d", &a, &b, &c);
    return ((uint32_t)a << 16) | ((uint32_t)b << 8) | (uint32_t)c;
  }
  bool available() const { return latest.length() && ver(latest) > ver(FW_VERSION); }

  bool check() {
    lastCheckMs = millis(); checked = true; error = "";
    String url;
    if (!resolve("https://github.com/" FW_REPO "/releases/latest/download/version.json", url)) return false;
    String body;
    {
      std::unique_ptr<BearSSL::X509List> cas;
      std::unique_ptr<BearSSL::WiFiClientSecure> client;
      if (!secureClient(GH_CA_ASSETS, cas, client)) return false;
      client->setBufferSizes(8192, 512);  // tiny file: handshake records are <= ~4 KB
      HTTPClient http;
      http.setTimeout(10000);
      if (!http.begin(*client, url)) { error = "begin"; return false; }
      int code = timed([&] { return http.GET(); });
      if (code != HTTP_CODE_OK) { error = fail(code, http, *client); http.end(); return false; }
      body = http.getString();
      http.end();
    }
    JsonDocument doc;
    if (deserializeJson(doc, body) || !doc["version"].is<const char *>()) { error = "json"; return false; }
    latest = doc["version"].as<const char *>();
    return true;
  }

  // Step 1 (normal mode): follow github.com's redirects to the short-lived CDN URL of the image.
  bool assetUrl(String &url) {
    if (!available()) { error = "no update"; return false; }
    return resolve("https://github.com/" FW_REPO "/releases/download/v" + latest + "/" FW_ASSET, url);
  }

  // Step 2 (update boot, unfragmented heap): one TLS session to the CDN.
  // Never returns on success (reboots into the new image).
  bool install(const String &url, std::function<void(int, int)> progress) {
    std::unique_ptr<BearSSL::X509List> cas;
    std::unique_ptr<BearSSL::WiFiClientSecure> client;
    if (!secureClient(GH_CA_ASSETS, cas, client)) return false;  // default 16 KB receive buffer
    ESPhttpUpdate.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    ESPhttpUpdate.rebootOnUpdate(true);
    ESPhttpUpdate.onProgress(progress);
    heapAtConnect = ESP.getFreeHeap(); blockAtConnect = ESP.getMaxFreeBlockSize();
    t_httpUpdate_return r = timed([&] { return ESPhttpUpdate.update(*client, url); });
    if (r != HTTP_UPDATE_OK) { error = ESPhttpUpdate.getLastErrorString() + sslError(*client); return false; }
    return true;
  }

private:
  // The CDN chain ends in ISRG Root X1 (RSA-4096): verifying it takes longer than the 3 s soft
  // watchdog even at 160 MHz. The hardware watchdog (~8 s) stays armed.
  template <typename F> static auto timed(F f) -> decltype(f()) {
    ESP.wdtDisable();
    auto r = f();
    ESP.wdtEnable(0);
    return r;
  }

  // github.com/.../latest/download/x -> github.com/.../download/vX/x -> asset CDN.
  // Follows the github.com hops in a small session (released before returning) and stops at the CDN URL.
  bool resolve(String url, String &location) {
    std::unique_ptr<BearSSL::X509List> cas;
    std::unique_ptr<BearSSL::WiFiClientSecure> client;
    if (!secureClient(GH_CA_GITHUB, cas, client)) return false;
    client->setBufferSizes(8192, 512);
    for (uint8_t hop = 0; hop < 4; hop++) {
      HTTPClient http;
      http.setTimeout(10000);
      http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
      const char *keep[] = {"Location"};
      http.collectHeaders(keep, 1);
      if (!http.begin(*client, url)) { error = "begin"; return false; }
      heapAtConnect = ESP.getFreeHeap(); blockAtConnect = ESP.getMaxFreeBlockSize();
      int code = timed([&] { return http.GET(); });
      if (code != HTTP_CODE_FOUND && code != HTTP_CODE_MOVED_PERMANENTLY && code != HTTP_CODE_TEMPORARY_REDIRECT) {
        error = code == HTTP_CODE_OK ? String("no redirect") : fail(code, http, *client);
        http.end();
        return false;
      }
      location = http.header("Location");
      http.end();
      if (location.startsWith("https://github.com/")) { url = location; continue; }
      if (location.startsWith("https://")) return true;
      error = "redirect";
      return false;
    }
    error = "too many redirects";
    return false;
  }

  bool secureClient(const char *pemP, std::unique_ptr<BearSSL::X509List> &cas, std::unique_ptr<BearSSL::WiFiClientSecure> &client) {
    time_t now = time(nullptr);
    if (now < 1700000000) { error = "no time yet"; return false; }  // certificate dates need a valid clock
    {
      String pem = FPSTR(pemP);  // PROGMEM -> RAM only while parsing
      cas.reset(new BearSSL::X509List(pem.c_str()));
    }
    client.reset(new BearSSL::WiFiClientSecure());
    client->setTrustAnchors(cas.get());
    client->setX509Time(now);
    return true;
  }

  String fail(int code, HTTPClient &http, BearSSL::WiFiClientSecure &c) {
    return code < 0 ? http.errorToString(code) + sslError(c) : "HTTP " + String(code);
  }

  static String sslError(BearSSL::WiFiClientSecure &c) {
    char b[80];
    int e = c.getLastSSLError(b, sizeof(b));
    return e ? " (TLS " + String(e) + ": " + b + ")" : "";
  }
};
