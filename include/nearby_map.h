#pragma once

#include <Arduino.h>
#include "web_ui.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
using MapServer = ESP8266WebServer;
extern "C" {
#include "user_interface.h"
}
#else
#include <WiFi.h>
#include <BLEDevice.h>
#include <esp_wifi.h>
using MapServer = WebServer;
#endif

static constexpr uint8_t MAP_WIFI_MAX = 24;
static constexpr uint8_t MAP_BLE_MAX = 16;
static constexpr uint8_t MAP_CLIENT_MAX = 8;

static inline uint16_t mapStableAngle(const String &key) {
  uint32_t h = 2166136261u;
  for (unsigned i = 0; i < key.length(); i++) {
    h ^= static_cast<uint8_t>(key[i]);
    h *= 16777619u;
  }
  return static_cast<uint16_t>(h % 360);
}

static inline String mapJsonEscape(const String &s) {
  String o;
  o.reserve(s.length() + 8);
  for (unsigned i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '\\' || c == '"') {
      o += '\\';
      o += c;
    } else if (c < 0x20) {
      o += ' ';
    } else {
      o += c;
    }
  }
  return o;
}

static inline int mapScanNetworkCount() {
#if defined(ESP8266)
  return WiFi.scanNetworks();
#else
  return WiFi.scanNetworks(false, true);
#endif
}

static inline void mapScanNetworksCleanup() {
#if !defined(ESP8266)
  WiFi.scanDelete();
#endif
}

static inline const char *mapRssiRangeLabel(int rssi) {
  if (rssi >= -45) {
    return "~2m";
  }
  if (rssi >= -60) {
    return "~10m";
  }
  if (rssi >= -75) {
    return "~30m";
  }
  return "100m+";
}

static inline bool mapLooksLikeTribeNode(const String &ssid, const String &selfAp) {
  if (ssid.length() == 0) {
    return false;
  }
  if (ssid == selfAp) {
    return true;
  }
  return ssid.indexOf('|') >= 0 || ssid.indexOf("repeater") >= 0 ||
         ssid.indexOf("Tribe") >= 0 || ssid.indexOf("tribe") >= 0 ||
         ssid.indexOf("TribeHub") >= 0 || ssid.indexOf("TribeNode") >= 0;
}

#if !defined(ESP8266)
static bool mapBleInited = false;

static inline void mapBleInitOnce() {
  if (!mapBleInited) {
    BLEDevice::init("");
    mapBleInited = true;
  }
}

static inline void mapAppendBle(String &json, bool &first) {
  mapBleInitOnce();
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(80);
  scan->setWindow(60);
  BLEScanResults *results = scan->start(2, false);
  if (results == nullptr) {
    return;
  }
  const int n =
      min(static_cast<int>(results->getCount()), static_cast<int>(MAP_BLE_MAX));
  for (int i = 0; i < n; i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);
    if (!first) {
      json += ',';
    }
    first = false;
    String name = dev.getName().c_str();
    if (name.length() == 0) {
      name = "(anonymous)";
    }
    const String addr = dev.getAddress().toString().c_str();
    const int rssi = dev.getRSSI();
    json += "{\"kind\":\"ble\",\"name\":\"" + mapJsonEscape(name) + "\",\"addr\":\"" +
            mapJsonEscape(addr) + "\",\"rssi\":" + String(rssi) + ",\"range\":\"" +
            mapJsonEscape(String(mapRssiRangeLabel(rssi))) + "\",\"angle\":" +
            String(mapStableAngle(addr)) + "}";
  }
  scan->clearResults();
}
#endif

static inline void mapAppendApClients(String &json, bool &first) {
#if defined(ESP8266)
  uint8_t count = 0;
  for (struct station_info *info = wifi_softap_get_station_info();
       info != nullptr && count < MAP_CLIENT_MAX; info = info->next.stqe_next) {
    char mac[20];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", info->bssid[0],
             info->bssid[1], info->bssid[2], info->bssid[3], info->bssid[4],
             info->bssid[5]);
    if (!first) {
      json += ',';
    }
    first = false;
    const String macStr(mac);
    json += "{\"kind\":\"client\",\"name\":\"" + mapJsonEscape(macStr) +
            "\",\"rssi\":-45,\"range\":\"~2m\",\"angle\":" +
            String(mapStableAngle(macStr)) + "}";
    count++;
  }
  wifi_softap_free_station_info();
#else
  wifi_sta_list_t staList = {};
  if (esp_wifi_ap_get_sta_list(&staList) != ESP_OK) {
    return;
  }
  for (int i = 0; i < staList.num && i < MAP_CLIENT_MAX; i++) {
    char mac[20];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", staList.sta[i].mac[0],
             staList.sta[i].mac[1], staList.sta[i].mac[2], staList.sta[i].mac[3],
             staList.sta[i].mac[4], staList.sta[i].mac[5]);
    if (!first) {
      json += ',';
    }
    first = false;
    const String macStr(mac);
    const int rssi = staList.sta[i].rssi;
    json += "{\"kind\":\"client\",\"name\":\"" + mapJsonEscape(macStr) +
            "\",\"rssi\":" + String(rssi) + ",\"range\":\"" +
            mapJsonEscape(String(mapRssiRangeLabel(rssi))) + "\",\"angle\":" +
            String(mapStableAngle(macStr)) + "}";
  }
#endif
}

static inline String mapBuildScanJson(const String &selfAp, bool privacyRedact = false) {
  const int n = mapScanNetworkCount();
  String json = "{\"self\":{\"ssid\":\"" + mapJsonEscape(selfAp) + "\"}";
#if defined(ESP8266)
  json += ",\"hasBle\":false,\"radio\":\"WiFi only (ESP8266)\"";
#else
  json += ",\"hasBle\":true,\"radio\":\"WiFi + BLE (ESP32)\"";
#endif
  json += ",\"ranges\":["
          "{\"rssi\":-45,\"label\":\"~2m\"},"
          "{\"rssi\":-60,\"label\":\"~10m\"},"
          "{\"rssi\":-75,\"label\":\"~30m\"},"
          "{\"rssi\":-88,\"label\":\"100m+\"}"
          "],\"nodes\":[";
  bool first = true;

  if (!privacyRedact) {
    mapAppendApClients(json, first);
  }

  int wifiAdded = 0;
  for (int i = 0; i < n && wifiAdded < MAP_WIFI_MAX; i++) {
    const String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;
    }
    if (!first) {
      json += ',';
    }
    first = false;
    wifiAdded++;
    const int rssi = WiFi.RSSI(i);
    const bool tribe = mapLooksLikeTribeNode(ssid, selfAp);
    json += "{\"kind\":\"wifi\",\"name\":\"" + mapJsonEscape(ssid) + "\",\"rssi\":" +
            String(rssi) + ",\"ch\":" + String(WiFi.channel(i)) +
            ",\"secure\":" +
            String(
#if defined(ESP8266)
                WiFi.encryptionType(i) != ENC_TYPE_NONE
#else
                WiFi.encryptionType(i) != WIFI_AUTH_OPEN
#endif
                    ? 1
                    : 0) +
            ",\"tribe\":" + String(tribe ? 1 : 0) + ",\"angle\":" +
            String(mapStableAngle(ssid)) + ",\"range\":\"" +
            mapJsonEscape(String(mapRssiRangeLabel(rssi))) + "\"}";
  }
  mapScanNetworksCleanup();

#if !defined(ESP8266)
  mapAppendBle(json, first);
#endif

  json += "]}";
  return json;
}

static inline String mapPageBody() {
  String body;
  body.reserve(900);
  body += F("<p class=\"chat-warn\">Radar map from live <b>WiFi</b> scan");
#if !defined(ESP8266)
  body += F(" + <b>BLE</b>");
#endif
  body += F(". RSSI sets distance (not GPS). Angles are hashed — not real direction.</p>");
  body += F("<p class=\"chat-meta\"><span style=\"color:#f60\">●</span> tribe/repeater SSID "
            "<span style=\"color:#94f\">●</span> WiFi "
            "<span style=\"color:#93f\">●</span> AP client");
#if !defined(ESP8266)
  body += F(" <span style=\"color:#c6f\">●</span> BLE");
#endif
  body += F("</p>");
  body += F("<p class=\"chat-meta map-ranges\">Rings: <b>~2m</b> −45 dBm · <b>~10m</b> −60 · "
            "<b>~30m</b> −75 · <b>100m+</b> −88 (estimate from RSSI)</p>");
  body += F("<canvas id=\"nodeMap\" class=\"node-map\" width=\"320\" height=\"320\"></canvas>");
  body += F("<ul id=\"mapList\" class=\"map-list\"></ul>");
  body += F("<p class=\"chat-meta\" id=\"mapMeta\">Scanning…</p>");
  body += F("<button type=\"button\" id=\"mapRefresh\" style=\"max-width:12rem;margin:.5rem auto 0\">"
            "Rescan</button>");
  body += FPSTR(WEB_UI_MAP_SCRIPT);
  return body;
}

static inline void mapHandlePage(MapServer &server, const char *apSsid,
                                 const char *pageTitle) {
  server.send(200, "text/html", htmlPage(mapPageBody(), apSsid, pageTitle));
}

static inline void mapHandleData(MapServer &server, const String &selfAp, bool privacyRedact = false) {
  server.send(200, "application/json", mapBuildScanJson(selfAp, privacyRedact));
}
