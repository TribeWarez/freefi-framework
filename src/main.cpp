#include <Arduino.h>
#include <Wire.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "SSD1306Wire.h"
#include "config.h"
#include "web_ui.h"
#include "web_chat.h"
#include "nearby_map.h"
#include "oled_screens.h"
#include "oled_init.h"
#include "wifi_platform.h"
#include "wifi_store.h"
#include "ap_store.h"
#include "serial_cli.h"
#include "credential_crypto.h"

#if defined(ESP8266)
#include <ESP8266WebServer.h>
using HttpServer = ESP8266WebServer;
#else
#include <Network.h>
#include <WebServer.h>
using HttpServer = WebServer;
#endif

static SSD1306Wire *display = nullptr;
static HttpServer server(80);

static String uplinkSsid;
static String uplinkPass;
static String apSsid;
static String apPass;
static String pageTitle;
static WifiPlatformState wifiState;
static OledScreenState oledState;
static OledMetrics oledMetrics;
static SerialCliContext serialCli;

static void onStaGotIp();
static void onStaDisconnected();
static void connectUplink();
static void refreshOledMetrics();
static void startWebServer();
static void restartAccessPoint();
static ApPlatformOptions currentApOptions();

static ApPlatformOptions currentApOptions() {
  ApPlatformOptions opt;
  opt.hidden = apStoreApHidden();
  opt.clientIsolate = false;
  return opt;
}

static void onStaGotIp() {
#if defined(TW_DEBUG_WIFI)
  Serial.printf("STA IP: %s  RSSI: %d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
#else
  Serial.println(F("Uplink: connected"));
#endif
  wifiPlatformEnableNat(wifiState, true);
  wifiPlatformSetApDnsFromSta();
  refreshOledMetrics();
}

static void onStaDisconnected() {
  wifiPlatformEnableNat(wifiState, false);
  static uint32_t lastDiscLogMs = 0;
  const uint32_t now = millis();
  if (now - lastDiscLogMs > 15000) {
    Serial.println(F("Uplink: disconnected"));
    lastDiscLogMs = now;
  }
  refreshOledMetrics();
}

static void refreshOledMetrics() {
  oledMetrics.naptEnabled = wifiState.naptEnabled;
  oledMetrics.privacyRedact = apStorePrivacyRedact();
  oledMetrics.apSsid = apSsid.c_str();
  oledMetrics.uplinkSsid = &uplinkSsid;
  oledScreensForceDraw(display, oledState, oledMetrics);
}

static void restartAccessPoint() {
  WiFi.softAPdisconnect(true);
  delay(100);
  wifiPlatformInitAp(apSsid.c_str(), apPass.c_str(), currentApOptions());
#if defined(TW_DEBUG_WIFI)
  Serial.printf("AP restarted: %s\n", apSsid.c_str());
#else
  Serial.println(F("AP restarted"));
#endif
}

static void connectUplink() {
  if (uplinkSsid.length() == 0) {
    return;
  }
  wifiPlatformEnableNat(wifiState, false);
  WiFi.disconnect(true);
  delay(100);
  wifiPlatformRandomizeStaMac();
#if defined(TW_DEBUG_WIFI)
  Serial.printf("STA connect: %s\n", uplinkSsid.c_str());
#else
  Serial.println(F("STA connect"));
#endif
  WiFi.begin(uplinkSsid.c_str(), uplinkPass.c_str());
}

static void loadWifiCredentials() {
  loadWifiConfig(uplinkSsid, uplinkPass);
  if (uplinkSsid.length() > 0 && !credStringIsValid(uplinkSsid)) {
    uplinkSsid = "";
    uplinkPass = "";
    clearWifiConfig();
    Serial.println(F("Cleared invalid uplink in flash — set again: uplink <ssid> <pass>"));
  }
}

static void saveWifiCredentials(const String &ssid, const String &pass) {
  saveWifiConfig(ssid, pass);
  uplinkSsid = ssid;
  uplinkPass = pass;
}

static String uplinkStatusHtml() {
  if (WiFi.status() == WL_CONNECTED) {
    return "<p><b>Uplink:</b> connected<br>IP " + WiFi.localIP().toString() + " RSSI " +
           String(WiFi.RSSI()) + " dBm<br>NAT: " +
           String(wifiState.naptEnabled ? "active" : "off") + "</p>";
  }
  if (uplinkSsid.length() > 0) {
    if (apStorePrivacyRedact()) {
      return "<p><b>Uplink:</b> connecting…</p>";
    }
    return "<p><b>Uplink:</b> connecting to <code>" + uplinkSsid + "</code>…</p>";
  }
  return "<p><b>Uplink:</b> not configured.</p>";
}

static void handleOperatorBlocked() {
  const String body =
      "<p><b>Operator settings</b></p>"
      "<p>WiFi credentials and AP admin are not available on the web UI.</p>"
      "<p>Connect USB serial at <b>115200</b> baud and type <code>help</code>.</p>"
      "<p>Guests: <a href=\"/\">home</a> · <a href=\"/chat\">chat</a> · "
      "<a href=\"/map\">map</a></p>";
  server.send(200, "text/html", htmlPage(body, apSsid.c_str(), pageTitle.c_str()));
}

// ── Web UI ──────────────────────────────────────────────────────────────────

static void handleRoot() {
  const String body = uplinkStatusHtml() + "<p><b>Hotspot AP:</b> <code>" + apSsid +
                      "</code><br>AP IP <code>" + WiFi.softAPIP().toString() +
                      "</code><br>Clients: " + String(WiFi.softAPgetStationNum()) +
                      "</p><p><a href=\"/chat\">IRC chat</a> · <a href=\"/map\">Nearby node map</a> "
                      "(WiFi"
#if !defined(ESP8266)
                      " + BLE"
#endif
                      ")</p>";
  server.send(200, "text/html", htmlPage(body, apSsid.c_str(), pageTitle.c_str()));
}

static void handleFavicon() {
#if defined(ESP8266)
  server.send_P(200, PSTR("image/svg+xml"), WEB_FAVICON_SVG);
#else
  server.send_P(200, "image/svg+xml", WEB_FAVICON_SVG);
#endif
}

static void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

static void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/chat", HTTP_GET, []() {
    chatHandlePage(server, apSsid.c_str(), pageTitle.c_str());
  });
  server.on("/chat/poll", HTTP_GET, []() { chatHandlePoll(server); });
  server.on("/chat/send", HTTP_POST, []() { chatHandleSend(server); });
  server.on("/map", HTTP_GET, []() {
    mapHandlePage(server, apSsid.c_str(), pageTitle.c_str());
  });
  server.on("/map/data", HTTP_GET, []() {
    mapHandleData(server, apSsid, apStorePrivacyRedact());
  });
  server.on("/config", HTTP_GET, handleOperatorBlocked);
  server.on("/settings", HTTP_GET, handleOperatorBlocked);
  server.on("/settings/save", HTTP_POST, handleOperatorBlocked);
  server.on("/scan", HTTP_GET, handleOperatorBlocked);
  server.on("/save", HTTP_POST, handleOperatorBlocked);
  server.on("/favicon.ico", HTTP_GET, handleFavicon);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("HTTP server on port 80"));
}

// ── Arduino ─────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(300);
#if defined(ESP8266)
  Serial.println(F("\nESP8266 NAT repeater + Tribe node"));
#elif defined(ZY_ESP32_BOARD)
  Serial.println(F("\nZY-ESP32 NAT repeater + Tribe node"));
#else
  Serial.println(F("\nESP32 NAT repeater + Tribe node"));
#endif

  loadApSettings(apSsid, apPass, pageTitle);
  if (apSsid.length() == 0) {
    apSsid = "TribeNode-" + String(static_cast<uint32_t>(credChipId() & 0xFFFF), HEX);
    if (apPass.length() == 0 && AP_PASS[0] != '\0') {
      apPass = AP_PASS;
    }
    saveApSettings(apSsid, apPass, pageTitle);
  }
  chatInit();

  display = initOledAutodetect();
  oledRandomSeed();
  loadWifiCredentials();
  wifiPlatformRegisterEvents(wifiState, onStaGotIp, onStaDisconnected);

  wifiPlatformInitAp(apSsid.c_str(), apPass.c_str(), currentApOptions());
#if defined(TW_DEBUG_WIFI)
  Serial.printf("AP: %s  http://%s\n", apSsid.c_str(), WiFi.softAPIP().toString().c_str());
#else
  Serial.print(F("AP up  http://"));
  Serial.println(WiFi.softAPIP());
#endif
  startWebServer();

  serialCli.uplinkSsid = &uplinkSsid;
  serialCli.uplinkPass = &uplinkPass;
  serialCli.apSsid = &apSsid;
  serialCli.apPass = &apPass;
  serialCli.pageTitle = &pageTitle;
  serialCli.wifiState = &wifiState;
  serialCli.onSaveUplink = [](const String &ssid, const String &pass) {
    saveWifiCredentials(ssid, pass);
    connectUplink();
    refreshOledMetrics();
  };
  serialCli.onClearUplink = []() {
    clearWifiConfig();
    uplinkSsid = "";
    uplinkPass = "";
    WiFi.disconnect(true);
    wifiPlatformEnableNat(wifiState, false);
    refreshOledMetrics();
  };
  serialCli.onSaveAp = []() { refreshOledMetrics(); };
  serialCli.onRestartAp = []() { restartAccessPoint(); };
  serialCli.onConnectUplink = []() { connectUplink(); };
  serialCli.onRefreshOled = []() { refreshOledMetrics(); };
  serialCliPrintHelp();

  if (uplinkSsid.length() > 0) {
    connectUplink();
  } else {
    Serial.println(F("Serial: uplink <ssid> <pass>  —  type help"));
  }

  oledMetrics.naptEnabled = wifiState.naptEnabled;
  oledMetrics.privacyRedact = apStorePrivacyRedact();
  oledMetrics.apSsid = apSsid.c_str();
  oledMetrics.uplinkSsid = &uplinkSsid;
  oledState.lastSwitchMs = millis();
  oledScreensForceDraw(display, oledState, oledMetrics);
}

void loop() {
  server.handleClient();
  serialCliPoll(serialCli);

  const uint32_t nowMs = millis();
  for (uint8_t i = 0; i < CHAT_MAX_USERS; i++) {
    if (chatUsers[i].hash != 0 && nowMs - chatUsers[i].lastSeenMs > CHAT_USER_TTL_MS) {
      chatUsers[i] = {};
    }
  }

  oledMetrics.naptEnabled = wifiState.naptEnabled;
  oledMetrics.privacyRedact = apStorePrivacyRedact();
  oledMetrics.apSsid = apSsid.c_str();
  oledMetrics.uplinkSsid = &uplinkSsid;
  oledScreensTick(display, oledState, oledMetrics);

  static uint32_t lastRetry = 0;
  if (uplinkSsid.length() > 0 && WiFi.status() != WL_CONNECTED &&
      millis() - lastRetry > 30000) {
    lastRetry = millis();
    connectUplink();
  }
}
