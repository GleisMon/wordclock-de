#pragma once
// Firmware updates from GitHub Releases.
// Each release carries version.json ({"version":"1.2.3"}) and the signed wortuhr-de.bin.
// Two layers of protection: TLS is verified against the root CAs in certs.h, and the
// Updater only accepts images RSA-signed with our key (public.key, see README).
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

  static uint32_t ver(const String &v) {  // "1.2.3" -> 0x010203
    int a = 0, b = 0, c = 0;
    sscanf(v.c_str(), "%d.%d.%d", &a, &b, &c);
    return ((uint32_t)a << 16) | ((uint32_t)b << 8) | (uint32_t)c;
  }
  bool available() const { return latest.length() && ver(latest) > ver(FW_VERSION); }

  bool check() {
    lastCheckMs = millis(); checked = true; error = "";
    std::unique_ptr<BearSSL::X509List> cas;
    std::unique_ptr<BearSSL::WiFiClientSecure> client;
    if (!secureClient(cas, client)) return false;
    // GitHub's handshake records are <= ~4 KB and version.json is tiny, so 8 KB fits next to the
    // running web server. The firmware download (16 KB records) runs in the lean update boot instead.
    client->setBufferSizes(8192, 512);
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(10000);
    if (!http.begin(*client, "https://github.com/" FW_REPO "/releases/latest/download/version.json")) { error = "begin"; return false; }
    heapAtConnect = ESP.getFreeHeap();
    int code = http.GET();
    if (code != HTTP_CODE_OK) { error = code < 0 ? http.errorToString(code) + sslError(*client) : "HTTP " + String(code); http.end(); return false; }
    JsonDocument doc;
    DeserializationError e = deserializeJson(doc, http.getString());
    http.end();
    if (e || !doc["version"].is<const char *>()) { error = "json"; return false; }
    latest = doc["version"].as<const char *>();
    return true;
  }

  // Never returns on success (reboots into the new image).
  bool install(std::function<void(int, int)> progress) {
    if (!available()) { error = "no update"; return false; }
    std::unique_ptr<BearSSL::X509List> cas;
    std::unique_ptr<BearSSL::WiFiClientSecure> client;
    if (!secureClient(cas, client)) return false;
    ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    ESPhttpUpdate.rebootOnUpdate(true);
    ESPhttpUpdate.onProgress(progress);
    String url = "https://github.com/" FW_REPO "/releases/download/v" + latest + "/" FW_ASSET;
    heapAtConnect = ESP.getFreeHeap();
    t_httpUpdate_return r = ESPhttpUpdate.update(*client, url);
    if (r != HTTP_UPDATE_OK) { error = ESPhttpUpdate.getLastErrorString() + sslError(*client); return false; }
    return true;
  }

  uint32_t heapAtConnect = 0;

private:
  static String sslError(BearSSL::WiFiClientSecure &c) {
    char b[80];
    int e = c.getLastSSLError(b, sizeof(b));
    return e ? " (TLS " + String(e) + ": " + b + ")" : "";
  }

  bool secureClient(std::unique_ptr<BearSSL::X509List> &cas, std::unique_ptr<BearSSL::WiFiClientSecure> &client) {
    time_t now = time(nullptr);
    if (now < 1700000000) { error = "no time yet"; return false; }  // certificate dates need a valid clock
    {
      String pem = FPSTR(GH_ROOT_CAS);  // PROGMEM -> RAM only while parsing
      cas.reset(new BearSSL::X509List(pem.c_str()));
    }
    client.reset(new BearSSL::WiFiClientSecure());
    client->setTrustAnchors(cas.get());
    client->setX509Time(now);
    return true;
  }
};
