#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#include <esp_wifi.h>
#endif

struct WifiPlatformState {
  bool naptEnabled = false;
  bool naptAvailable = false;
};

struct ApPlatformOptions {
  bool hidden = false;
  bool clientIsolate = false;
};

#if defined(ESP8266)

#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
#include "lwip/napt.h"
}

static inline void wifiPlatformEnableNat(WifiPlatformState &state, bool on) {
#if defined(IP_NAPT) && IP_NAPT
  if (on && WiFi.status() == WL_CONNECTED) {
    ip_napt_enable(WiFi.softAPIP(), 1);
    state.naptEnabled = true;
    state.naptAvailable = true;
#if defined(TW_DEBUG_WIFI)
    Serial.println("NAPT enabled (ESP8266 lwIP)");
#endif
  } else {
    ip_napt_enable(WiFi.softAPIP(), 0);
    state.naptEnabled = false;
#if defined(TW_DEBUG_WIFI)
    Serial.println("NAPT disabled");
#endif
  }
#else
  state.naptEnabled = false;
  state.naptAvailable = false;
  Serial.println("NAPT not compiled in — AP clients: local web only");
#endif
}

static inline void wifiPlatformSetApDnsFromSta() {
  (void)WiFi.dnsIP();
}

static inline void wifiPlatformInitAp(const char *ssid, const char *pass,
                                      const ApPlatformOptions &opt = ApPlatformOptions()) {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));
  const int hidden = opt.hidden ? 1 : 0;
  if (pass != nullptr && pass[0] != '\0') {
    WiFi.softAP(ssid, pass, 1, hidden, 4);
  } else {
    WiFi.softAP(ssid, nullptr, 1, hidden, 4);
  }
  if (opt.clientIsolate) {
    Serial.println("AP client isolation not available on ESP8266");
  }
}

static inline void wifiPlatformRandomizeStaMac() {
#if !defined(STA_MAC_RANDOM) || STA_MAC_RANDOM
  uint8_t mac[6];
  for (uint8_t i = 0; i < 6; i++) {
    mac[i] = static_cast<uint8_t>(random(256));
  }
  mac[0] = (mac[0] & 0xFC) | 0x02;
  wifi_set_macaddr(STATION_IF, mac);
#endif
}

static inline void wifiPlatformRegisterEvents(WifiPlatformState &state,
                                              void (*onGotIp)(),
                                              void (*onDisconnected)()) {
  WiFi.onStationModeGotIP([onGotIp](const WiFiEventStationModeGotIP &) {
    if (onGotIp) {
      onGotIp();
    }
  });
  WiFi.onStationModeDisconnected(
      [onDisconnected](const WiFiEventStationModeDisconnected &) {
        if (onDisconnected) {
          onDisconnected();
        }
      });
  (void)state;
}

#else  // ESP32

#include <Network.h>
#include <esp_netif.h>

static inline void wifiPlatformEnableNat(WifiPlatformState &state, bool on) {
  WiFi.AP.enableNAPT(on);
  state.naptEnabled = on && WiFi.status() == WL_CONNECTED;
  state.naptAvailable = true;
#if defined(TW_DEBUG_WIFI)
  if (state.naptEnabled) {
    Serial.println("NAPT enabled — AP clients can use uplink internet");
  } else {
    Serial.println("NAPT disabled");
  }
#endif
}

static inline void wifiPlatformSetApDnsFromSta() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap == nullptr) {
    return;
  }
  esp_netif_dns_info_t dns = {};
  dns.ip.type = ESP_IPADDR_TYPE_V4;
  dns.ip.u_addr.ip4.addr = static_cast<uint32_t>(WiFi.dnsIP());
  esp_netif_set_dns_info(ap, ESP_NETIF_DNS_MAIN, &dns);
}

static inline void wifiPlatformApplyApIsolate(bool on) {
  wifi_config_t cfg = {};
  if (esp_wifi_get_config(WIFI_IF_AP, &cfg) != ESP_OK) {
    return;
  }
#if defined(CONFIG_ESP_WIFI_SOFTAP_SUPPORT) || 1
  // isolate field removed in newer IDF; try struct member if present at compile time
#endif
  (void)on;
  // Best-effort: some IDF builds expose no AP isolate API — guests rely on map redaction
}

static inline void wifiPlatformInitAp(const char *ssid, const char *pass,
                                      const ApPlatformOptions &opt = ApPlatformOptions()) {
  WiFi.mode(WIFI_AP_STA);
  WiFi.AP.begin();
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));
  const int hidden = opt.hidden ? 1 : 0;
  if (pass != nullptr && pass[0] != '\0') {
    WiFi.softAP(ssid, pass, 1, hidden, 4);
  } else {
    WiFi.softAP(ssid, nullptr, 1, hidden, 4);
  }
  if (opt.clientIsolate) {
    wifiPlatformApplyApIsolate(true);
  }
}

static inline void wifiPlatformRandomizeStaMac() {
#if !defined(STA_MAC_RANDOM) || STA_MAC_RANDOM
  uint8_t mac[6];
  for (uint8_t i = 0; i < 6; i++) {
    mac[i] = static_cast<uint8_t>(esp_random() & 0xFF);
  }
  mac[0] = (mac[0] & 0xFC) | 0x02;
  esp_wifi_set_mac(WIFI_IF_STA, mac);
#endif
}

static inline void wifiPlatformRegisterEvents(WifiPlatformState &state,
                                              void (*onGotIp)(),
                                              void (*onDisconnected)()) {
  (void)state;
  Network.onEvent([onGotIp, onDisconnected](arduino_event_id_t event,
                                              arduino_event_info_t info) {
    (void)info;
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP && onGotIp) {
      onGotIp();
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && onDisconnected) {
      onDisconnected();
    }
  });
}

#endif
